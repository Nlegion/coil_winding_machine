#include "position.h"
#include "motors.h"
#include "endstops.h"
#include "logger.h"
#include "storage.h"
#include "web_server.h"

// Глобальные переменные
long currentPosSteps = 0;
bool systemHomed = false;

void initPosition() {
  // Позиция инициализируется при загрузке из EEPROM
}

float getCurrentPosMM() {
  // Конвертация шагов в миллиметры
  return (float)currentPosSteps / STEPS_PER_MM;
}

long getCurrentPosSteps() {
  return currentPosSteps;
}

void setPosition(float posMM) {
  currentPosSteps = (long)(posMM * STEPS_PER_MM);
}

void setPositionSteps(long steps) {
  currentPosSteps = steps;
}

bool isSystemHomed() {
  return systemHomed;
}

void setSystemHomed(bool homed) {
  systemHomed = homed;
}

bool moveLinearTo(float targetMM, int speed) {
  // Функция для ручного перемещения суппорта
  if (!systemHomed) {
    lastError = "Система не откалибрована";
    return false;
  }
  
  long targetSteps = (long)(targetMM * STEPS_PER_MM);
  long distanceSteps = targetSteps - currentPosSteps;
  
  if (abs(distanceSteps) < 1) return true; // Уже на месте
  
  enableMotors(true);
  setLinearDirection(distanceSteps > 0);
  
  unsigned long startTime = millis();
  long stepsToMove = abs(distanceSteps);
  
  for (long i = 0; i < stepsToMove; i++) {
    // Проверка таймаута
    if (millis() - startTime > MOVE_TIMEOUT_MS) {
      enableMotors(false);
      lastError = "Таймаут движения";
      return false;
    }
    
    // Проверка концевиков с debounce
    updateEndstops();
    
    if (isHomePressed() || isEndPressed()) {
      enableMotors(false);
      lastError = "Сработал концевой выключатель";
      return false;
    }
    
    stepLinearMotor(speed);
    currentPosSteps += (distanceSteps > 0) ? 1 : -1;
    
    // Периодический yield
    if (i % 100 == 0) {
      yield();
      server.handleClient();
    }
  }
  
  enableMotors(false);
  savePosition();
  return true;
}

