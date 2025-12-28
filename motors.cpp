#include "motors.h"
#include "logger.h"

void initMotors() {
  pinMode(MOTOR_ROT_STEP, OUTPUT);
  pinMode(MOTOR_ROT_DIR, OUTPUT);
  pinMode(MOTOR_LIN_STEP, OUTPUT);
  pinMode(MOTOR_LIN_DIR, OUTPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);
  
  digitalWrite(MOTOR_ENABLE, HIGH); // Выключить драйверы (активный LOW)
}

void enableMotors(bool enable) {
  digitalWrite(MOTOR_ENABLE, enable ? LOW : HIGH); // LOW = включено, HIGH = выключено
}

void setRotationalDirection(bool dir) {
  digitalWrite(MOTOR_ROT_DIR, dir ? HIGH : LOW);
}

void setLinearDirection(bool dir) {
  digitalWrite(MOTOR_LIN_DIR, dir ? HIGH : LOW);
}

void stepRotationalMotor(int delayMicros) {
  digitalWrite(MOTOR_ROT_STEP, HIGH);
  delayMicroseconds(delayMicros);
  digitalWrite(MOTOR_ROT_STEP, LOW);
  delayMicroseconds(delayMicros);
}

void stepLinearMotor(int delayMicros) {
  digitalWrite(MOTOR_LIN_STEP, HIGH);
  delayMicroseconds(delayMicros);
  digitalWrite(MOTOR_LIN_STEP, LOW);
  delayMicroseconds(delayMicros);
}

// Глобальные переменные для управления вращательным двигателем
static bool rotMotorRunning = false;
static bool rotMotorDirection = true; // true = по часовой
static int rotMotorSpeed = 1000; // мкс
static unsigned long rotMotorLastStepTime = 0;
static int rotMotorStepsRemaining = 0; // Оставшееся количество шагов (0 = бесконечное вращение)
static int rotMotorStepsTotal = 0; // Всего шагов для выполнения

void startRotationalMotor(bool direction, int speed) {
  rotMotorRunning = true;
  rotMotorDirection = direction;
  rotMotorSpeed = speed;
  rotMotorStepsRemaining = 0; // Бесконечное вращение
  rotMotorStepsTotal = 0;
  setRotationalDirection(direction);
  rotMotorLastStepTime = micros();
  enableMotors(true);
}

void startRotationalMotorSteps(bool direction, int speed, int steps) {
  // Останавливаем предыдущее вращение, если было
  if (rotMotorRunning) {
    rotMotorRunning = false;
    delay(10); // Небольшая задержка для завершения текущего шага
  }
  
  rotMotorRunning = true;
  rotMotorDirection = direction;
  rotMotorSpeed = speed;
  rotMotorStepsRemaining = steps;
  rotMotorStepsTotal = steps;
  setRotationalDirection(direction);
  rotMotorLastStepTime = micros();
  enableMotors(true);
  logInfo("Вращательный двигатель запущен: направление=" + String(direction ? "CW" : "CCW") + ", скорость=" + String(speed) + " мкс, шагов=" + String(steps));
}

void stopRotationalMotor() {
  rotMotorRunning = false;
  rotMotorStepsRemaining = 0;
  rotMotorStepsTotal = 0;
}

bool isRotationalMotorRunning() {
  return rotMotorRunning;
}

void updateRotationalMotor() {
  // Non-blocking обновление вращательного двигателя
  // Должно вызываться из loop()
  if (rotMotorRunning) {
    unsigned long currentTime = micros();
    // Проверяем переполнение micros() (происходит каждые ~70 минут)
    unsigned long elapsed = 0;
    if (currentTime >= rotMotorLastStepTime) {
      elapsed = currentTime - rotMotorLastStepTime;
    } else {
      // Переполнение произошло
      elapsed = (ULONG_MAX - rotMotorLastStepTime) + currentTime;
      rotMotorLastStepTime = currentTime;
    }
    
    if (elapsed >= rotMotorSpeed) {
      stepRotationalMotor(rotMotorSpeed / 2);
      rotMotorLastStepTime = currentTime;
      
      // Если задано ограниченное количество шагов, уменьшаем счетчик
      if (rotMotorStepsRemaining > 0) {
        rotMotorStepsRemaining--;
        if (rotMotorStepsRemaining <= 0) {
          // Достигнуто нужное количество шагов, останавливаем
          logInfo("Вращательный двигатель завершил " + String(rotMotorStepsTotal) + " шагов");
          rotMotorRunning = false;
          rotMotorStepsRemaining = 0;
          rotMotorStepsTotal = 0;
        }
      }
    }
  }
}

