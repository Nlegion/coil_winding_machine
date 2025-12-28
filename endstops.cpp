#include "endstops.h"
#include "logger.h"

// Глобальные переменные состояния концевиков
EndstopState endstopHome = {HIGH, 0, HIGH};
EndstopState endstopEnd = {HIGH, 0, HIGH};

void initEndstops() {
  pinMode(ENDSTOP_HOME, INPUT_PULLUP);
  pinMode(ENDSTOP_END, INPUT_PULLUP);
}

bool readEndstopDebounced(int pin, EndstopState& state) {
  bool currentState = digitalRead(pin);
  unsigned long currentTime = millis();
  
  if (currentState != state.lastState) {
    // Состояние изменилось, сбрасываем таймер
    state.lastChangeTime = currentTime;
    state.lastState = currentState;
  } else {
    // Состояние стабильно
    if (currentTime - state.lastChangeTime >= ENDSTOP_DEBOUNCE_MS) {
      // Прошло достаточно времени, обновляем debounced состояние
      state.debouncedState = currentState;
    }
  }
  
  return state.debouncedState == LOW; // Возвращает true если концевик нажат
}

bool isHomePressed() {
  return endstopHome.debouncedState == LOW;
}

bool isEndPressed() {
  return endstopEnd.debouncedState == LOW;
}

bool checkEndstopWarning(float posMM) {
  // Проверка приближения к концевикам
  float distToHome = posMM;
  float distToEnd = 500.0 - posMM; // Предполагаем максимальный ход 500мм
  
  if (distToHome < ENDSTOP_WARNING_DIST_MM || distToEnd < ENDSTOP_WARNING_DIST_MM) {
    if (distToHome < ENDSTOP_WARNING_DIST_MM) {
      logWarn("Приближение к HOME концевику: " + String(distToHome) + " мм");
    }
    if (distToEnd < ENDSTOP_WARNING_DIST_MM) {
      logWarn("Приближение к END концевику: " + String(distToEnd) + " мм");
    }
    return true;
  }
  
  return false;
}

void updateEndstops() {
  readEndstopDebounced(ENDSTOP_HOME, endstopHome);
  readEndstopDebounced(ENDSTOP_END, endstopEnd);
}

