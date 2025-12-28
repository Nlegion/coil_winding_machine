#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "types.h"

// Функции логирования
void logMessage(String level, String message);
void logError(String message);
void logWarn(String message);
void logInfo(String message);
void logDebug(String message);
String getErrorString(ErrorCode code);

// Глобальная переменная для последней ошибки
extern String lastError;

#endif // LOGGER_H

