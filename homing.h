#ifndef HOMING_H
#define HOMING_H

#include <Arduino.h>
#include "types.h"

// Начало процесса калибровки
void startCalibration();

// Non-blocking шаг калибровки (state machine)
bool calibrationStep();

// Блокирующая версия (для обратной совместимости)
void calibrateAxis();

// Проверка состояния
bool isCalibrating();
CalibrationState getCalibrationState();

// Глобальные переменные
extern CalibrationState calibrationState;
extern unsigned long calibrationStateStartTime;
extern unsigned long calibrationLastStepTime;
extern int calibrationBackoffSteps;

#endif // HOMING_H

