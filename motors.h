#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include "config.h"

// Инициализация двигателей
void initMotors();

// Управление драйверами
void enableMotors(bool enable);

// Управление направлением
void setRotationalDirection(bool dir);  // true = HIGH, false = LOW
void setLinearDirection(bool dir);      // true = HIGH, false = LOW

// Выполнение шагов
void stepRotationalMotor(int delayMicros);
void stepLinearMotor(int delayMicros);

// Управление непрерывным вращением
void startRotationalMotor(bool direction, int speed); // direction: true = по часовой, false = против часовой
void startRotationalMotorSteps(bool direction, int speed, int steps); // Вращение на указанное количество шагов
void stopRotationalMotor();
bool isRotationalMotorRunning();
void updateRotationalMotor(); // Non-blocking обновление (вызывать из loop)

#endif // MOTORS_H

