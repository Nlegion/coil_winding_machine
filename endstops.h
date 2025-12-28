#ifndef ENDSTOPS_H
#define ENDSTOPS_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// Инициализация концевиков
void initEndstops();

// Чтение концевиков с debounce
bool readEndstopDebounced(int pin, EndstopState& state);

// Проверка состояния концевиков
bool isHomePressed();
bool isEndPressed();

// Проверка приближения к концевику
bool checkEndstopWarning(float posMM);

// Обновление состояний концевиков
void updateEndstops();

// Глобальные переменные состояния концевиков
extern EndstopState endstopHome;
extern EndstopState endstopEnd;

#endif // ENDSTOPS_H

