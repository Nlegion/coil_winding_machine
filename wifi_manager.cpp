#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include <ESP8266WiFi.h>

// Глобальные переменные
String wifiSSID = "Homenet_plus";
String wifiPassword = "29pronto69";
unsigned long lastWiFiCheck = 0;

void setWiFiCredentials(String ssid, String password) {
  wifiSSID = ssid;
  wifiPassword = password;
}

void initWiFi() {
  // Подключение к WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  Serial.print("Подключение к WiFi");
  for (int i = 0; i < WIFI_TIMEOUT * 2; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Установка hostname для устройства в сети
    WiFi.hostname("coil_winding_machine");
    
    Serial.println("\nПодключено к WiFi!");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: coil_winding_machine");
    Serial.println();
    logInfo("WiFi подключен: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nНе удалось подключиться к WiFi");
    logError("WiFi не подключен. Проверьте SSID и пароль.");
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    logWarn("WiFi отключен, попытка переподключения...");
    
    WiFi.disconnect();
    delay(100);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_RECONNECT_ATTEMPTS * 10) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      // Установка hostname при переподключении
      WiFi.hostname("coil_winding_machine");
      logInfo("WiFi переподключен: " + WiFi.localIP().toString());
    } else {
      logError("Не удалось переподключиться к WiFi");
    }
  }
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String getWiFiIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  } else {
    return "Не подключен";
  }
}

int getWiFiRSSI() {
  return WiFi.RSSI();
}

