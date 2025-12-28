#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

// Инициализация WiFi
void initWiFi();

// Проверка и переподключение WiFi
void checkWiFiConnection();

// Проверка подключения
bool isWiFiConnected();

// Получение информации
String getWiFiIP();
int getWiFiRSSI();

// Настройка WiFi (можно вызвать перед initWiFi)
void setWiFiCredentials(String ssid, String password);

// Глобальные переменные
extern String wifiSSID;
extern String wifiPassword;
extern unsigned long lastWiFiCheck;

#endif // WIFI_MANAGER_H

