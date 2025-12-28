#ifndef POSITION_H
#define POSITION_H

#include <Arduino.h>
#include "config.h"

// Инициализация позиции
void initPosition();

// Получение текущей позиции
float getCurrentPosMM();
long getCurrentPosSteps();

// Установка позиции
void setPosition(float posMM);
void setPositionSteps(long steps);

// Перемещение к целевой позиции
bool moveLinearTo(float targetMM, int speed = 1000);

// Состояние системы
bool isSystemHomed();
void setSystemHomed(bool homed);

// Глобальные переменные
extern long currentPosSteps;
extern bool systemHomed;

#endif // POSITION_H

