/*
 * ПОЛНЫЙ КОД ДЛЯ СТАНКА НАМОТКИ КАТУШЕК
 * Управление через веб-интерфейс, два двигателя NEMA 17, два концевика SS-5GL.
 * Аппаратная часть: Wemos D1 Mini, 2x A4988, блок питания 12V.
 * Версия: 3.0 (Оптимизированная, Модульная)
 */

#include "config.h"
#include "types.h"
#include "logger.h"
#include "motors.h"
#include "endstops.h"
#include "position.h"
#include "homing.h"
#include "winding.h"
#include "storage.h"
#include "wifi_manager.h"
#include "web_server.h"

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  logInfo("=== ИНИЦИАЛИЗАЦИЯ СТАНКА ДЛЯ НАМОТКИ ===");

  // Инициализация модулей
  initMotors();
  initEndstops();
  logInfo("Пины сконфигурированы.");

  // Инициализация памяти
  initStorage();
  logInfo("Настройки загружены.");

  // Проверка памяти
  if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    logWarn("Мало свободной памяти: " + String(ESP.getFreeHeap()));
  }

  // Инициализация WiFi
  initWiFi();

  // Инициализация веб-сервера
  initWebServer();

  logInfo("====================================");
}

// ==================== LOOP ====================
void loop() {
  // Обработка веб-сервера
  handleWebClient();
  
  // Проверка WiFi периодически
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL_MS) {
    checkWiFiConnection();
    lastWiFiCheck = millis();
  }
  
  // Обновление debounce концевиков
  updateEndstops();
  
  // Обработка state machine для калибровки (non-blocking)
  if (isCalibrating()) {
    calibrationStep();
  }
  
  // Проверка концевиков в реальном времени (если намотка активна)
  if (windingActive) {
    if (isHomePressed() || isEndPressed()) {
      logError("Сработал концевой выключатель! Аварийная остановка.");
      emergencyStop = true;
      stopWindingProcess();
    } else {
      // Проверка предупреждения о приближении
      checkEndstopWarning(getCurrentPosMM());
    }
  }
  
  // Non-blocking обработка намотки
  updateWinding();
  
  // Обновление вращательного двигателя (для ручного управления)
  updateRotationalMotor();
  
  yield(); // Важно для ESP8266
}
