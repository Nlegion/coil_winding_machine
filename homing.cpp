#include "homing.h"
#include "config.h"
#include "motors.h"
#include "endstops.h"
#include "position.h"
#include "logger.h"
#include "storage.h"
#include "web_server.h"

// Глобальные переменные
CalibrationState calibrationState = CALIB_IDLE;
unsigned long calibrationStateStartTime = 0;
unsigned long calibrationLastStepTime = 0;
int calibrationBackoffSteps = 0;
float endPositionMM = 0.0; // Позиция END концевика для вычисления длины

void startCalibration() {
  calibrationState = CALIB_MOVING_TO_END;
  calibrationStateStartTime = millis();
  calibrationLastStepTime = micros();
  calibrationBackoffSteps = 0;
  enableMotors(true);
  setLinearDirection(true); // Движение вправо к END (дальний концевик)
  logInfo("Начало калибровки: поиск дальнего концевика (END)");
}

bool calibrationStep() {
  // Non-blocking версия калибровки на state machine
  const int stepDelay = 1500;  // мкс
  
  // Проверка таймаута
  unsigned long elapsedTime = millis() - calibrationStateStartTime;
  if (elapsedTime > HOMING_TIMEOUT_MS) {
    calibrationState = CALIB_ERROR;
    setSystemHomed(false);
    enableMotors(false);
    lastError = "Таймаут калибровки (прошло " + String(elapsedTime / 1000) + " сек, состояние: " + String(calibrationState) + ")";
    logError(lastError);
    return false;
  }
  
  // Обработка сервера и WiFi
  server.handleClient();
  yield();
  
  // Обновление debounce концевиков
  updateEndstops();
  
  switch (calibrationState) {
    case CALIB_MOVING_TO_END: {
      // Движение к END концевику (дальний)
      if (isEndPressed()) {
        // END концевик сработал - правильно
        calibrationState = CALIB_BACKING_OFF_END;
        calibrationBackoffSteps = 0;
        setLinearDirection(false); // Меняем направление (влево)
        calibrationStateStartTime = millis();
        calibrationLastStepTime = micros();
        logInfo("END концевик найден");
        return true;
      }
      
      // Проверка: если сработал HOME вместо END - концевики перепутаны
      if (isHomePressed()) {
        calibrationState = CALIB_ERROR;
        enableMotors(false);
        lastError = "Концевики перепутаны: при поиске END сработал HOME";
        logError(lastError);
        return false;
      }
      
      // Выполняем шаг
      if (micros() - calibrationLastStepTime >= stepDelay) {
        stepLinearMotor(stepDelay / 2);
        // Обновляем позицию: движение вправо (к END) увеличивает позицию
        currentPosSteps++;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_BACKING_OFF_END: {
      // Отъезд от END концевика
      // Сначала убеждаемся, что концевик отпущен
      if (calibrationBackoffSteps == 0 && isEndPressed()) {
        // Концевик все еще нажат, продолжаем отъезд
        if (micros() - calibrationLastStepTime >= 800) {
          stepLinearMotor(400);
          // Обновляем позицию: отъезд от END (влево) уменьшает позицию
          currentPosSteps--;
          calibrationBackoffSteps++;
          calibrationLastStepTime = micros();
        }
        break;
      }
      
      // Если концевик отпущен, продолжаем отъезд до целевого количества шагов
      if (calibrationBackoffSteps >= HOME_BACKOFF_STEPS_TARGET) {
        // Проверяем, что концевик точно отпущен перед переходом к следующему этапу
        if (!isEndPressed()) {
          calibrationState = CALIB_MOVING_TO_END_SLOW;
          setLinearDirection(true); // Возвращаемся к END
          calibrationStateStartTime = millis();
          calibrationLastStepTime = micros();
          logInfo("Отъезд от END завершен. Медленное движение к END...");
          return true;
        } else {
          // Концевик все еще нажат, продолжаем отъезд
          if (micros() - calibrationLastStepTime >= 800) {
            stepLinearMotor(400);
            // Обновляем позицию: отъезд от END (влево) уменьшает позицию
            currentPosSteps--;
            calibrationBackoffSteps++;
            calibrationLastStepTime = micros();
          }
        }
        break;
      }
      
      if (micros() - calibrationLastStepTime >= 800) {
        stepLinearMotor(400);
        // Обновляем позицию: отъезд от END (влево) уменьшает позицию
        currentPosSteps--;
        calibrationBackoffSteps++;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_MOVING_TO_END_SLOW: {
      // Медленное движение к END для точного позиционирования
      if (isEndPressed()) {
        // Точное позиционирование на END завершено
        // Сохраняем позицию END концевика
        endPositionMM = getCurrentPosMM();
        logInfo("END концевик найден (медленное движение). Позиция END: " + String(endPositionMM) + " мм. Отъезд от END перед поиском HOME...");
        calibrationState = CALIB_BACKING_OFF_END_FOR_HOME;
        setLinearDirection(false); // Движение влево (от END)
        calibrationStateStartTime = millis();
        calibrationLastStepTime = micros();
        calibrationBackoffSteps = 0;
        return true;
      }
      
      // Проверка: если сработал HOME вместо END - концевики перепутаны
      if (isHomePressed()) {
        calibrationState = CALIB_ERROR;
        enableMotors(false);
        lastError = "Концевики перепутаны: при возврате к END сработал HOME";
        logError(lastError);
        return false;
      }
      
      if (micros() - calibrationLastStepTime >= 2000) {
        stepLinearMotor(1000);
        // Обновляем позицию: медленное движение к END (вправо) увеличивает позицию
        currentPosSteps++;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_BACKING_OFF_END_FOR_HOME: {
      // Отъезд от END концевика перед движением к HOME
      // Отъезжаем достаточно далеко, чтобы концевик точно отпустился
      if (calibrationBackoffSteps >= HOME_BACKOFF_STEPS_TARGET) {
        // Проверяем, что END концевик отпущен
        if (!isEndPressed()) {
          // Теперь движемся к HOME концевику
          float currentPos = getCurrentPosMM();
          logInfo("Отъезд от END завершен. Начало движения к HOME концевику... Текущая позиция: " + String(currentPos) + " мм");
          calibrationState = CALIB_MOVING_TO_HOME;
          setLinearDirection(false); // Движение влево к HOME
          calibrationStateStartTime = millis();
          calibrationLastStepTime = micros();
          // Сброс статической переменной для логирования
          return true;
        } else {
          // Концевик все еще нажат, продолжаем отъезд
          if (micros() - calibrationLastStepTime >= 800) {
            stepLinearMotor(400);
            // Обновляем позицию: отъезд от END перед HOME (влево) уменьшает позицию
            currentPosSteps--;
            calibrationBackoffSteps++;
            calibrationLastStepTime = micros();
          }
        }
        break;
      }
      
      if (micros() - calibrationLastStepTime >= 800) {
        stepLinearMotor(400);
        // Обновляем позицию: отъезд от END перед HOME (влево) уменьшает позицию
        currentPosSteps--;
        calibrationBackoffSteps++;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_MOVING_TO_HOME: {
      // Движение к HOME концевику (ближний)
      // Периодическое логирование для отладки (каждые 5 секунд)
      static unsigned long lastLogTime = 0;
      static float lastPositionCheck = -1.0;
      static unsigned long lastPositionChangeTime = 0;
      
      unsigned long currentTime = millis();
      float currentPos = getCurrentPosMM();
      
      // Логирование каждые 5 секунд
      if (currentTime - lastLogTime > 5000 || lastLogTime == 0) {
        logInfo("Движение к HOME концевику... Позиция: " + String(currentPos) + " мм, HOME: " + String(isHomePressed() ? "нажат" : "отпущен") + ", END: " + String(isEndPressed() ? "нажат" : "отпущен"));
        lastLogTime = currentTime;
      }
      
      // Проверка застревания - если позиция не меняется более 10 секунд
      if (abs(currentPos - lastPositionCheck) < 0.1) {
        // Позиция не меняется
        if (lastPositionChangeTime == 0) {
          lastPositionChangeTime = currentTime;
        } else if (currentTime - lastPositionChangeTime > 10000) {
          // Застряли более 10 секунд
          calibrationState = CALIB_ERROR;
          enableMotors(false);
          lastError = "Движение к HOME остановилось. Позиция не меняется: " + String(currentPos) + " мм";
          logError(lastError);
          return false;
        }
      } else {
        // Позиция меняется, сбрасываем таймер
        lastPositionChangeTime = 0;
        lastPositionCheck = currentPos;
      }
      
      if (isHomePressed()) {
        // HOME концевик сработал - правильно
        logInfo("HOME концевик найден при движении влево. Позиция: " + String(currentPos) + " мм");
        calibrationState = CALIB_BACKING_OFF_HOME;
        calibrationBackoffSteps = 0;
        setLinearDirection(true); // Меняем направление (вправо) для отъезда
        calibrationStateStartTime = millis();
        calibrationLastStepTime = micros();
        return true;
      }
      
      // Проверка: если сработал END вместо HOME - концевики перепутаны
      // Но только если мы уже достаточно далеко от END (чтобы избежать ложных срабатываний)
      // При движении к HOME позиция должна уменьшаться, поэтому если END срабатывает
      // при большой позиции, это может быть проблемой
      if (isEndPressed()) {
        // Проверяем позицию - если мы еще близко к END, это может быть ложное срабатывание
        // Учитываем, что после отъезда от END мы можем быть еще в зоне его срабатывания
        // Поэтому проверяем только если позиция достаточно большая (далеко от END)
        if (currentPos > 50.0) {
          // Мы достаточно далеко от END, он не должен срабатывать
          // Это может означать, что концевики перепутаны
          calibrationState = CALIB_ERROR;
          enableMotors(false);
          lastError = "Концевики перепутаны: при поиске HOME сработал END (позиция: " + String(currentPos) + " мм)";
          logError(lastError);
          return false;
        } else {
          // Возможно, мы еще близко к END, игнорируем это срабатывание
          // Это нормально, если мы только что отъехали от END
          logWarn("END концевик сработал при движении к HOME, но позиция близка к END (" + String(currentPos) + " мм). Продолжаем движение...");
        }
      }
      
      // Выполняем шаг
      if (micros() - calibrationLastStepTime >= stepDelay) {
        stepLinearMotor(stepDelay / 2);
        // Обновляем позицию: движение к HOME (влево) уменьшает позицию
        currentPosSteps--;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_BACKING_OFF_HOME: {
      // Отъезд от HOME концевика
      if (calibrationBackoffSteps >= HOME_BACKOFF_STEPS_TARGET) {
        calibrationState = CALIB_MOVING_TO_HOME_SLOW;
        setLinearDirection(false); // Возвращаемся к HOME
        calibrationStateStartTime = millis();
        calibrationLastStepTime = micros();
        return true;
      }
      
      if (micros() - calibrationLastStepTime >= 800) {
        stepLinearMotor(400);
        // Обновляем позицию: отъезд от HOME (вправо) увеличивает позицию
        currentPosSteps++;
        calibrationBackoffSteps++;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_MOVING_TO_HOME_SLOW: {
      // Медленное движение к HOME для точного позиционирования
      if (isHomePressed()) {
        // Точное позиционирование завершено - HOME это начало (0 мм)
        setPositionSteps(0);
        setSystemHomed(true);
        calibrationState = CALIB_DONE;
        enableMotors(false);
        
        // Вычисляем и выводим длину между концевиками
        float distanceBetweenEndstops = endPositionMM;
        logInfo("Калибровка завершена. Позиция: 0 мм (HOME)");
        logInfo("=== РЕЗУЛЬТАТЫ КАЛИБРОВКИ ===");
        logInfo("Позиция HOME концевика: 0.00 мм");
        logInfo("Позиция END концевика: " + String(endPositionMM, 2) + " мм");
        logInfo("Длина между концевиками: " + String(distanceBetweenEndstops, 2) + " мм");
        logInfo("====================================");
        
        savePosition();
        return true;
      }
      
      if (micros() - calibrationLastStepTime >= 2000) {
        stepLinearMotor(1000);
        // Обновляем позицию: медленное движение к HOME (влево) уменьшает позицию
        currentPosSteps--;
        calibrationLastStepTime = micros();
      }
      break;
    }
    
    case CALIB_IDLE:
    case CALIB_DONE:
    case CALIB_ERROR:
      return false;
  }
  
  return true;
}

void calibrateAxis() {
  // Блокирующая версия для обратной совместимости
  startCalibration();
  
  while (calibrationState != CALIB_DONE && calibrationState != CALIB_ERROR) {
    calibrationStep();
    delay(1);
  }
}

bool isCalibrating() {
  return calibrationState != CALIB_IDLE && calibrationState != CALIB_DONE && calibrationState != CALIB_ERROR;
}

CalibrationState getCalibrationState() {
  return calibrationState;
}

