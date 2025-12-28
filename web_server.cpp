#include "web_server.h"
#include "config.h"
#include "types.h"
#include "logger.h"
#include "storage.h"
#include "position.h"
#include "endstops.h"
#include "homing.h"
#include "winding.h"
#include "wifi_manager.h"
#include "motors.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>

// Глобальная переменная сервера
ESP8266WebServer server(80);

// Вспомогательные функции
bool handleFileRead(String path);
String getContentType(String filename);
String getIndexHTML();

void handleAPI() {
  // Возвращает JSON с текущим состоянием системы
  StaticJsonDocument<1024> doc;
  
  doc["homed"] = isSystemHomed();
  doc["winding"] = windingActive;
  doc["emergency"] = emergencyStop;
  doc["position"] = getCurrentPosMM();
  doc["turns"] = turnsMade;
  doc["targetTurns"] = settings.targetTurns;
  doc["layer"] = currentLayer;
  doc["home_switch"] = isHomePressed();
  doc["end_switch"] = isEndPressed();
  doc["wire_length"] = calculateWireLength(turnsMade);
  doc["warning_near_endstop"] = checkEndstopWarning(getCurrentPosMM());
  doc["wifi_connected"] = isWiFiConnected();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["last_error"] = lastError;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSettings() {
  // Принимает и сохраняет новые настройки
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (!error) {
      // Сохраняем старые значения для валидации
      CoilSettings oldSettings = settings;
      
      if (doc.containsKey("wireDia")) settings.wireDia = doc["wireDia"];
      if (doc.containsKey("coreDia")) settings.coreDia = doc["coreDia"];
      if (doc.containsKey("coreLen")) settings.coreLen = doc["coreLen"];
      if (doc.containsKey("targetTurns")) settings.targetTurns = doc["targetTurns"];
      if (doc.containsKey("layers")) settings.layers = doc["layers"];
      if (doc.containsKey("speed")) settings.speed = doc["speed"];
      
      // Валидация параметров
      if (!validateSettings()) {
        // Восстанавливаем старые значения
        settings = oldSettings;
        server.send(400, "application/json", "{\"error\":\"Неверные параметры\",\"details\":\"" + lastError + "\"}");
        return;
      }
      
      saveSettings();
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Нет данных\"}");
  }
}

void handleCommand() {
  // Обработка команд (старт, стоп, домой и т.д.)
  if (!server.hasArg("cmd")) {
    server.send(400, "application/json", "{\"error\":\"Нет команды\"}");
    return;
  }
  
  String command = server.arg("cmd");
  String stepsParam = server.hasArg("steps") ? server.arg("steps") : "не указано";
  logInfo("Получена команда: " + command + ", шагов: " + stepsParam);
  
  if (command == "calibrate" || command == "home") {
    startCalibration();
    server.send(200, "application/json", "{\"status\":\"Калибровка начата\"}");
    
  } else if (command == "start") {
    if (!isSystemHomed()) {
      server.send(400, "application/json", "{\"error\":\"Сначала выполните калибровку\"}");
    } else if (windingActive) {
      server.send(400, "application/json", "{\"error\":\"Намотка уже идет\"}");
    } else if (emergencyStop) {
      server.send(400, "application/json", "{\"error\":\"Сбросьте аварийную остановку\"}");
    } else {
      startWindingProcess();
      server.send(200, "application/json", "{\"status\":\"Намотка начата\"}");
    }
    
  } else if (command == "stop") {
    stopWindingProcess();
    server.send(200, "application/json", "{\"status\":\"Намотка остановлена\"}");
    
  } else if (command == "reset_emergency") {
    emergencyStop = false;
    lastError = "";
    server.send(200, "application/json", "{\"status\":\"Аварийная остановка сброшена\"}");
    
  } else if (command == "move_to_home") {
    // Движение к HOME концевику на указанное количество шагов
    if (!isSystemHomed()) {
      server.send(400, "application/json", "{\"error\":\"Сначала выполните калибровку\"}");
    } else {
      int steps = 1600; // По умолчанию
      if (server.hasArg("steps")) {
        steps = server.arg("steps").toInt();
      }
      float targetMM = getCurrentPosMM() - (float)steps / STEPS_PER_MM;
      if (targetMM < 0) targetMM = 0;
      logInfo("Движение к HOME: " + String(steps) + " шагов, целевая позиция: " + String(targetMM) + " мм");
      if (!moveLinearTo(targetMM)) {
        server.send(400, "application/json", "{\"error\":\"Движение не выполнено\",\"details\":\"" + lastError + "\"}");
      } else {
        server.send(200, "application/json", "{\"status\":\"Движение к HOME выполнено (" + String(steps) + " шагов)\"}");
      }
    }
    
  } else if (command == "move_to_end") {
    // Движение к END концевику на указанное количество шагов
    if (!isSystemHomed()) {
      server.send(400, "application/json", "{\"error\":\"Сначала выполните калибровку\"}");
    } else {
      int steps = 1600; // По умолчанию
      if (server.hasArg("steps")) {
        steps = server.arg("steps").toInt();
      }
      float targetMM = getCurrentPosMM() + (float)steps / STEPS_PER_MM;
      logInfo("Движение к END: " + String(steps) + " шагов, целевая позиция: " + String(targetMM) + " мм");
      if (!moveLinearTo(targetMM)) {
        server.send(400, "application/json", "{\"error\":\"Движение не выполнено\",\"details\":\"" + lastError + "\"}");
      } else {
        server.send(200, "application/json", "{\"status\":\"Движение к END выполнено (" + String(steps) + " шагов)\"}");
      }
    }
    
  } else if (command == "rot_cw") {
    // Вращение по часовой стрелке на указанное количество шагов
    int steps = 3200; // По умолчанию (1 оборот)
    if (server.hasArg("steps")) {
      steps = server.arg("steps").toInt();
      if (steps <= 0) steps = 3200; // Защита от некорректных значений
    }
    logInfo("Запуск вращения по часовой: " + String(steps) + " шагов");
    startRotationalMotorSteps(true, 1000, steps);
    server.send(200, "application/json", "{\"status\":\"Вращение по часовой начато (" + String(steps) + " шагов)\"}");
    
  } else if (command == "rot_ccw") {
    // Вращение против часовой стрелки на указанное количество шагов
    int steps = 3200; // По умолчанию (1 оборот)
    if (server.hasArg("steps")) {
      steps = server.arg("steps").toInt();
      if (steps <= 0) steps = 3200; // Защита от некорректных значений
    }
    logInfo("Запуск вращения против часовой: " + String(steps) + " шагов");
    startRotationalMotorSteps(false, 1000, steps);
    server.send(200, "application/json", "{\"status\":\"Вращение против часовой начато (" + String(steps) + " шагов)\"}");
    
  } else if (command == "rot_stop") {
    // Остановка вращения
    stopRotationalMotor();
    server.send(200, "application/json", "{\"status\":\"Вращение остановлено\"}");
    
  } else {
    server.send(400, "application/json", "{\"error\":\"Неизвестная команда\"}");
  }
}

void handleSystem() {
  // Возвращает системную информацию в JSON формате
  StaticJsonDocument<512> doc;
  
  doc["version"] = "3.0";
  doc["ip"] = getWiFiIP();
  doc["wifi_rssi"] = getWiFiRSSI();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime_ms"] = millis();
  doc["wifi_connected"] = isWiFiConnected();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

bool handleFileRead(String path) {
  // Вспомогательная функция для отдачи файлов из SPIFFS
  if (path.endsWith("/")) {
    path = "/index.html";
  }
  
  // Убеждаемся, что путь начинается с /
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  
  #ifdef DEBUG_MODE
  logDebug("Запрос файла: " + path);
  #endif
  
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    if (file) {
      server.streamFile(file, getContentType(path));
      file.close();
      #ifdef DEBUG_MODE
      logDebug("Файл отправлен: " + path);
      #endif
      return true;
    } else {
      #ifdef DEBUG_MODE
      logError("Не удалось открыть файл: " + path);
      #endif
    }
  } else {
    #ifdef DEBUG_MODE
    logWarn("Файл не найден в SPIFFS: " + path);
    #endif
  }
  return false;
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  return "text/plain";
}

void initWebServer() {
  // Инициализация файловой системы (для веб-интерфейса)
  if (SPIFFS.begin()) {
    logInfo("Файловая система SPIFFS готова.");
    
    // Отладочная информация о файлах в SPIFFS
    #ifdef DEBUG_MODE
    Dir dir = SPIFFS.openDir("/");
    int fileCount = 0;
    while (dir.next()) {
      fileCount++;
      logDebug("SPIFFS файл: " + dir.fileName() + " (" + String(dir.fileSize()) + " байт)");
    }
    logInfo("Всего файлов в SPIFFS: " + String(fileCount));
    if (!SPIFFS.exists("/index.html")) {
      logWarn("ВНИМАНИЕ: /index.html не найден в SPIFFS!");
    }
    #endif
  } else {
    logError("Ошибка инициализации SPIFFS!");
  }

  // Настройка mDNS (для доступа по http://coil_winding_machine.local)
  if (MDNS.begin("coil_winding_machine")) {
    logInfo("mDNS запущен: http://coil_winding_machine.local");
  }

  // Настройка маршрутов веб-сервера
  // Обработчик для корневого пути - сначала пробуем SPIFFS, потом встроенную страницу
  server.on("/", HTTP_GET, []() {
    if (SPIFFS.exists("/index.html")) {
      handleFileRead("/index.html");
    } else {
      server.send(200, "text/html", getIndexHTML());
    }
  });
  
  // Обработчик для /index.html
  server.on("/index.html", HTTP_GET, []() {
    if (SPIFFS.exists("/index.html")) {
      handleFileRead("/index.html");
    } else {
      server.send(200, "text/html", getIndexHTML());
    }
  });
  
  // Статические файлы из папок
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/js", SPIFFS, "/js");
  
  // API маршруты
  server.on("/api", HTTP_GET, handleAPI);
  server.on("/settings", HTTP_POST, handleSettings);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/system", HTTP_GET, handleSystem);
  
  // Обработчик для всех остальных путей
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "Файл не найден: " + server.uri());
    }
  });

  server.begin();
  logInfo("HTTP сервер запущен.");
}

String getIndexHTML() {
  // Встроенная HTML страница с интерфейсом управления
  String html = "";
  html += "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Станок для намотки катушек</title>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px}.container{max-width:800px;margin:0 auto;background:white;border-radius:10px;padding:30px;box-shadow:0 10px 30px rgba(0,0,0,0.3)}h1{color:#333;margin-bottom:30px;text-align:center}.status{background:#f5f5f5;padding:15px;border-radius:5px;margin-bottom:20px}.status-item{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #ddd}.status-item:last-child{border-bottom:none}.status-label{font-weight:bold;color:#666}.status-value{color:#333}.status-value.on{color:#4caf50;font-weight:bold}.status-value.off{color:#f44336;font-weight:bold}.endstop-indicator{display:inline-block;width:20px;height:20px;border-radius:50%;margin-left:10px;vertical-align:middle}.endstop-indicator.active{background:#4caf50}.endstop-indicator.inactive{background:#ccc}.controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:20px}button{padding:12px 20px;border:none;border-radius:5px;font-size:16px;cursor:pointer;transition:all 0.3s;font-weight:bold}button:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2)}button:active{transform:translateY(0)}.btn-primary{background:#4caf50;color:white}.btn-primary:hover{background:#45a049}.btn-danger{background:#f44336;color:white}.btn-danger:hover{background:#da190b}.btn-warning{background:#ff9800;color:white}.btn-warning:hover{background:#e68900}.btn-info{background:#2196F3;color:white}.btn-info:hover{background:#0b7dda}.settings{margin-top:30px;padding-top:20px;border-top:2px solid #eee}.settings h2{margin-bottom:15px;color:#333}.form-group{margin-bottom:15px}label{display:block;margin-bottom:5px;color:#666;font-weight:bold}input{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;font-size:14px}.btn-save{background:#2196F3;color:white;width:100%;margin-top:10px}.message{padding:10px;margin:10px 0;border-radius:5px;display:none}.message.success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}.message.error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}</style>";
  html += "</head><body><div class=\"container\"><h1>Станок для намотки катушек</h1>";
  html += "<div id=\"message\" class=\"message\"></div>";
  html += "<div class=\"status\" id=\"status\">";
  html += "<div class=\"status-item\"><span class=\"status-label\">Статус:</span><span class=\"status-value\" id=\"statusText\">Загрузка...</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">Позиция:</span><span class=\"status-value\" id=\"position\">-</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">Витков:</span><span class=\"status-value\" id=\"turns\">-</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">Слой:</span><span class=\"status-value\" id=\"layer\">-</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">Длина провода:</span><span class=\"status-value\" id=\"wireLength\">-</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">WiFi:</span><span class=\"status-value\" id=\"wifiStatus\">-</span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">HOME концевик:</span><span class=\"status-value\">";
  html += "<span id=\"homeSwitchStatus\">-</span><span class=\"endstop-indicator inactive\" id=\"homeIndicator\"></span></span></div>";
  html += "<div class=\"status-item\"><span class=\"status-label\">END концевик:</span><span class=\"status-value\">";
  html += "<span id=\"endSwitchStatus\">-</span><span class=\"endstop-indicator inactive\" id=\"endIndicator\"></span></span></div>";
  html += "</div>";
  html += "<div class=\"controls\">";
  html += "<button class=\"btn-info\" onclick=\"sendCommand('calibrate')\">Калибровка</button>";
  html += "<button class=\"btn-primary\" onclick=\"sendCommand('start')\">Старт</button>";
  html += "<button class=\"btn-danger\" onclick=\"sendCommand('stop')\">Стоп</button>";
  html += "<button class=\"btn-warning\" onclick=\"sendCommand('reset_emergency')\">Сброс аварии</button>";
  html += "</div>";
  html += "<div class=\"settings\" style=\"margin-top:20px;\">";
  html += "<h3 style=\"margin-bottom:10px;color:#333;\">Ручное управление двигателями</h3>";
  html += "<div class=\"form-group\"><label>Количество шагов:</label><input type=\"number\" id=\"manualSteps\" step=\"1\" min=\"1\" max=\"100000\" value=\"1600\"></div>";
  html += "<div class=\"controls\" style=\"grid-template-columns:repeat(2,1fr);\">";
  html += "<button class=\"btn-info\" style=\"width:100%;\" onclick=\"sendCommandWithSteps('move_to_home',document.getElementById('manualSteps').value)\">К HOME</button>";
  html += "<button class=\"btn-info\" style=\"width:100%;\" onclick=\"sendCommandWithSteps('move_to_end',document.getElementById('manualSteps').value)\">К END</button>";
  html += "<button class=\"btn-info\" style=\"width:100%;\" onclick=\"sendCommandWithSteps('rot_cw',document.getElementById('manualSteps').value)\">Вращать по часовой</button>";
  html += "<button class=\"btn-info\" style=\"width:100%;\" onclick=\"sendCommandWithSteps('rot_ccw',document.getElementById('manualSteps').value)\">Вращать против часовой</button>";
  html += "</div>";
  html += "<button class=\"btn-danger\" style=\"width:100%;margin-top:10px;\" onclick=\"sendCommand('rot_stop')\">Стоп вращение</button>";
  html += "</div>";
  html += "<div class=\"settings\"><h2>Настройки</h2>";
  html += "<div class=\"form-group\"><label>Диаметр провода (мм):</label><input type=\"number\" id=\"wireDia\" step=\"0.01\" min=\"0.05\" max=\"2.0\" value=\"0.5\"></div>";
  html += "<div class=\"form-group\"><label>Диаметр сердечника (мм):</label><input type=\"number\" id=\"coreDia\" step=\"0.1\" min=\"1\" max=\"200\" value=\"20.0\"></div>";
  html += "<div class=\"form-group\"><label>Длина намотки (мм):</label><input type=\"number\" id=\"coreLen\" step=\"0.1\" min=\"1\" max=\"500\" value=\"50.0\"></div>";
  html += "<div class=\"form-group\"><label>Количество витков:</label><input type=\"number\" id=\"targetTurns\" step=\"1\" min=\"1\" max=\"10000\" value=\"200\"></div>";
  html += "<div class=\"form-group\"><label>Скорость (мкс):</label><input type=\"number\" id=\"speed\" step=\"10\" min=\"100\" max=\"5000\" value=\"600\"></div>";
  html += "<button class=\"btn-save\" onclick=\"saveSettings()\">Сохранить настройки</button></div></div>";
  html += "<script>let updateInterval;";
  html += "function showMessage(text,type){var msg=document.getElementById('message');msg.textContent=text;msg.className='message '+type;msg.style.display='block';setTimeout(function(){msg.style.display='none';},5000);}";
  html += "function sendCommand(cmd){fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+cmd}).then(function(r){return r.json();}).then(function(data){if(data.error){showMessage('Ошибка: '+data.error,'error');}else{showMessage(data.status||'Команда выполнена','success');}updateStatus();}).catch(function(e){showMessage('Ошибка связи: '+e,'error');});}";
  html += "function sendCommandWithSteps(cmd,steps){fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+cmd+'&steps='+steps}).then(function(r){return r.json();}).then(function(data){if(data.error){showMessage('Ошибка: '+data.error,'error');}else{showMessage(data.status||'Команда выполнена','success');}updateStatus();}).catch(function(e){showMessage('Ошибка связи: '+e,'error');});}";
  html += "function saveSettings(){var settings={wireDia:parseFloat(document.getElementById('wireDia').value),coreDia:parseFloat(document.getElementById('coreDia').value),coreLen:parseFloat(document.getElementById('coreLen').value),targetTurns:parseInt(document.getElementById('targetTurns').value),speed:parseInt(document.getElementById('speed').value)};fetch('/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(settings)}).then(function(r){return r.json();}).then(function(data){if(data.error){showMessage('Ошибка: '+data.error,'error');}else{showMessage('Настройки сохранены','success');}}).catch(function(e){showMessage('Ошибка связи: '+e,'error');});}";
  html += "function updateStatus(){fetch('/api').then(function(r){return r.json();}).then(function(data){document.getElementById('statusText').textContent=data.winding?'Намотка активна':data.emergency?'Аварийная остановка':data.homed?'Готов':'Не откалиброван';document.getElementById('statusText').className='status-value '+(data.winding?'on':'off');document.getElementById('position').textContent=data.position.toFixed(2)+' мм';document.getElementById('turns').textContent=data.turns+' / '+data.targetTurns;document.getElementById('layer').textContent=data.layer;document.getElementById('wireLength').textContent=data.wire_length.toFixed(2)+' мм';document.getElementById('wifiStatus').textContent=data.wifi_connected?'Подключен':'Отключен';document.getElementById('wifiStatus').className='status-value '+(data.wifi_connected?'on':'off');";
  html += "var homeIndicator=document.getElementById('homeIndicator');var endIndicator=document.getElementById('endIndicator');var homeStatus=document.getElementById('homeSwitchStatus');var endStatus=document.getElementById('endSwitchStatus');";
  html += "if(data.home_switch){homeIndicator.className='endstop-indicator active';homeStatus.textContent='Нажат';}else{homeIndicator.className='endstop-indicator inactive';homeStatus.textContent='Отпущен';}";
  html += "if(data.end_switch){endIndicator.className='endstop-indicator active';endStatus.textContent='Нажат';}else{endIndicator.className='endstop-indicator inactive';endStatus.textContent='Отпущен';}";
  html += "}).catch(function(e){console.error('Ошибка обновления:',e);});}";
  html += "function loadSettings(){fetch('/api').then(function(r){return r.json();}).then(function(data){if(data.targetTurns){document.getElementById('targetTurns').value=data.targetTurns;}});}";
  html += "updateInterval=setInterval(updateStatus,500);updateStatus();loadSettings();</script></body></html>";
  return html;
}

void handleWebClient() {
  server.handleClient();
  MDNS.update();
}
