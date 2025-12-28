#ifndef WINDING_H
#define WINDING_H

#include <Arduino.h>
#include "types.h"

// Управление процессом намотки
void startWindingProcess();
void stopWindingProcess();

// Non-blocking шаг одного витка
bool makeOneTurnStep(int direction);

// Блокирующая версия (для совместимости)
bool makeOneTurn(int direction);

// Расчет длины провода
float calculateWireLength(int turns);

// Обновление состояния намотки (вызывается из loop)
void updateWinding();

// Глобальные переменные состояния
extern bool windingActive;
extern bool windingInProgress;
extern bool emergencyStop;
extern int turnsMade;
extern int currentLayer;
extern WindingState windingState;
extern int windingTargetTurns;
extern float windingTurnsPerLayer;
extern int windingLayerDirection;
extern int windingTurnsInLayer;
extern int windingTurnsInLayerDone;

#endif // WINDING_H

