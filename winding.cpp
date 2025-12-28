#include "winding.h"
#include "config.h"
#include "motors.h"
#include "endstops.h"
#include "position.h"
#include "logger.h"
#include "storage.h"
#include "web_server.h"

// Глобальные переменные состояния
bool windingActive = false;
bool windingInProgress = false;
bool emergencyStop = false;
int turnsMade = 0;
int currentLayer = 0;
WindingState windingState;
int windingTargetTurns = 0;
float windingTurnsPerLayer = 0;
int windingLayerDirection = 1;
int windingTurnsInLayer = 0;
int windingTurnsInLayerDone = 0;

void startWindingProcess() {
  if (!isSystemHomed() || emergencyStop) {
    logError("Невозможно начать намотку: система не готова");
    return;
  }
  
  // Валидация параметров перед началом
  if (settings.wireDia <= 0 || settings.coreLen <= 0) {
    lastError = "Неверные параметры: wireDia или coreLen <= 0";
    logError(lastError);
    emergencyStop = true;
    return;
  }
  
  logInfo("=== НАЧАЛО ПРОЦЕССА НАМОТКИ ===");
  windingActive = true;
  emergencyStop = false;
  turnsMade = 0;
  currentLayer = 0;
  enableMotors(true);
  
  // Расчет параметров
  windingTurnsPerLayer = settings.coreLen / settings.wireDia;
  windingTargetTurns = settings.targetTurns;
  windingLayerDirection = 1; // Начинаем с направления вправо
  windingTurnsInLayerDone = 0;
  
  // Инициализация состояния намотки
  windingState.currentStep = 0;
  windingState.linearStepAccum = 0.0;
  windingState.linearStepsDone = 0;
  windingState.lastStepTime = micros();
  windingState.totalStepsForWire = (int)(settings.wireDia * STEPS_PER_MM);
  
  int remainingTurns = windingTargetTurns;
  windingTurnsInLayer = min((int)windingTurnsPerLayer, remainingTurns);
  
  setLinearDirection(true); // Направление вправо
  setRotationalDirection(true); // Направление вращения
  
  windingInProgress = true;
  
  logInfo("Параметры:");
  Serial.print("  Витков в слое: "); Serial.println(windingTurnsPerLayer);
  Serial.print("  Шаг провода: "); Serial.print(settings.wireDia); Serial.println(" мм");
  Serial.print("  Всего витков: "); Serial.println(windingTargetTurns);
}

bool makeOneTurnStep(int direction) {
  // Non-blocking версия makeOneTurn
  // Возвращает true если виток еще не завершен, false если завершен
  
  if (!windingActive || emergencyStop) {
    windingInProgress = false;
    return false;
  }
  
  // Проверка концевиков
  if (isHomePressed() || isEndPressed()) {
    emergencyStop = true;
    windingInProgress = false;
    return false;
  }
  
  unsigned long currentTime = micros();
  int stepDelay = settings.speed;
  
  // Проверка времени для следующего шага
  if (currentTime - windingState.lastStepTime < stepDelay) {
    return true; // Еще не время для шага
  }
  
  // Выполняем шаг вращения
  stepRotationalMotor(stepDelay / 2);
  
  windingState.currentStep++;
  windingState.lastStepTime = currentTime;
  
  // Линейное перемещение (интерполяция Брезенхема)
  if (windingState.totalStepsForWire > 0) {
    // Накопление дробных шагов
    float stepsPerRotStep = (float)windingState.totalStepsForWire / TOTAL_STEPS_PER_REV;
    windingState.linearStepAccum += stepsPerRotStep;
    
    // Если накопилось >= 1 шаг, выполняем линейный шаг
    if (windingState.linearStepAccum >= 1.0) {
      stepLinearMotor(stepDelay / 2);
      
      windingState.linearStepsDone++;
      windingState.linearStepAccum -= 1.0;
    }
  }
  
  // Проверка завершения оборота
  if (windingState.currentStep >= TOTAL_STEPS_PER_REV) {
    // Виток завершен, сброс состояния
    windingState.currentStep = 0;
    windingState.linearStepAccum = 0.0;
    windingState.linearStepsDone = 0;
    return false; // Виток завершен
  }
  
  // Периодический yield для стабильности
  if (windingState.currentStep % 100 == 0) {
    yield();
    server.handleClient();
  }
  
  return true; // Виток продолжается
}

bool makeOneTurn(int direction) {
  // Блокирующая версия для обратной совместимости
  windingState.currentStep = 0;
  windingState.linearStepAccum = 0.0;
  windingState.linearStepsDone = 0;
  windingState.lastStepTime = micros();
  windingState.totalStepsForWire = (int)(settings.wireDia * STEPS_PER_MM);
  
  while (makeOneTurnStep(direction)) {
    // Ждем завершения витка
    delayMicroseconds(10);
  }
  
  return !emergencyStop;
}

float calculateWireLength(int turns) {
  // Расчет общей длины провода: length = π * coreDia * turns
  if (turns <= 0 || settings.coreDia <= 0) return 0.0;
  return 3.14159265359 * settings.coreDia * turns;
}

void updateWinding() {
  // Обновление состояния намотки (вызывается из loop)
  if (windingInProgress && !emergencyStop) {
    // Выполняем один шаг намотки
    if (!makeOneTurnStep(windingLayerDirection)) {
      // Виток завершен
      turnsMade++;
      setPositionSteps(getCurrentPosSteps() + (long)(settings.wireDia * STEPS_PER_MM * windingLayerDirection));
      
      // Вывод прогресса каждые 10 витков
      if (turnsMade % 10 == 0) {
        Serial.print("  Прогресс: ");
        Serial.print(turnsMade);
        Serial.print("/");
        Serial.print(windingTargetTurns);
        Serial.print(" витков (");
        Serial.print((turnsMade * 100) / windingTargetTurns);
        Serial.println("%)");
      }
      
      windingTurnsInLayerDone++;
      
      // Проверка завершения слоя
      if (windingTurnsInLayerDone >= windingTurnsInLayer) {
        currentLayer++;
        windingTurnsInLayerDone = 0;
        
        // Проверка завершения всех витков
        if (turnsMade >= windingTargetTurns) {
          // Намотка завершена
          enableMotors(false);
          windingActive = false;
          windingInProgress = false;
          logInfo("Намотка успешно завершена! Всего витков: " + String(turnsMade));
          savePosition();
        } else {
          // Переход к следующему слою
          windingLayerDirection = (currentLayer % 2 == 0) ? 1 : -1;
          setLinearDirection(windingLayerDirection == 1);
          int remainingTurns = windingTargetTurns - turnsMade;
          windingTurnsInLayer = min((int)windingTurnsPerLayer, remainingTurns);
        }
      }
    }
  }
}

void stopWindingProcess() {
  windingActive = false;
  windingInProgress = false;
  emergencyStop = true;
  enableMotors(false);
  logInfo("Намотка принудительно остановлена.");
  savePosition();
}

