#include "logger.h"
#include "config.h"

String lastError = "";

void logMessage(String level, String message) {
  if (!DEBUG_MODE && level != "ERROR" && level != "WARN") return;
  
  Serial.print("[");
  Serial.print(level);
  Serial.print("] ");
  Serial.println(message);
}

void logError(String message) {
  lastError = message;
  logMessage("ERROR", message);
}

void logWarn(String message) {
  logMessage("WARN", message);
}

void logInfo(String message) {
  logMessage("INFO", message);
}

void logDebug(String message) {
  logMessage("DEBUG", message);
}

String getErrorString(ErrorCode code) {
  switch (code) {
    case ERROR_NONE: return "Нет ошибки";
    case ERROR_HOMING_TIMEOUT: return "Таймаут поиска дома";
    case ERROR_MOVE_TIMEOUT: return "Таймаут движения";
    case ERROR_ENDSTOP_TRIGGERED: return "Сработал концевой выключатель";
    case ERROR_INVALID_PARAMS: return "Неверные параметры";
    case ERROR_MEMORY_LOW: return "Мало памяти";
    case ERROR_WIFI_DISCONNECTED: return "WiFi отключен";
    default: return "Неизвестная ошибка";
  }
}

