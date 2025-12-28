#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <Arduino.h>

// Инициализация веб-сервера
void initWebServer();

// Обработка клиентов
void handleWebClient();

// Глобальная переменная сервера
extern ESP8266WebServer server;

#endif // WEB_SERVER_H

