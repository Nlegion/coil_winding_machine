#ifndef TYPES_H
#define TYPES_H

// ==================== СТРУКТУРЫ ДАННЫХ ====================
struct CoilSettings {
  float wireDia = 0.5;      // Диаметр провода (мм)
  float coreDia = 20.0;     // Диаметр сердечника (мм)
  float coreLen = 50.0;     // Длина намотки (мм)
  int targetTurns = 200;    // Желаемое число витков
  int layers = 1;           // Количество слоев
  int speed = 600;          // Скорость намотки (мкс между импульсами STEP)
  bool savePosition = true; // Сохранять позицию после отключения
};

// Состояния для state machine калибровки
enum CalibrationState {
  CALIB_IDLE,
  CALIB_MOVING_TO_END,        // Движение к дальнему концевику (END)
  CALIB_BACKING_OFF_END,      // Отъезд от END
  CALIB_MOVING_TO_END_SLOW,   // Медленное движение к END для точности
  CALIB_BACKING_OFF_END_FOR_HOME, // Отъезд от END перед движением к HOME
  CALIB_MOVING_TO_HOME,       // Движение к ближнему концевику (HOME)
  CALIB_BACKING_OFF_HOME,     // Отъезд от HOME
  CALIB_MOVING_TO_HOME_SLOW,  // Медленное движение к HOME для точности
  CALIB_DONE,
  CALIB_ERROR
};

// Коды ошибок
enum ErrorCode {
  ERROR_NONE = 0,
  ERROR_HOMING_TIMEOUT,
  ERROR_MOVE_TIMEOUT,
  ERROR_ENDSTOP_TRIGGERED,
  ERROR_INVALID_PARAMS,
  ERROR_MEMORY_LOW,
  ERROR_WIFI_DISCONNECTED
};

// Структура для debounce концевиков
struct EndstopState {
  bool lastState;
  unsigned long lastChangeTime;
  bool debouncedState;
};

// Структура для состояния намотки (non-blocking)
struct WindingState {
  int currentStep;           // Текущий шаг вращения (0-3199)
  float linearStepAccum;     // Накопление дробных линейных шагов
  int linearStepsDone;       // Выполнено целых линейных шагов
  unsigned long lastStepTime; // Время последнего шага
  int totalStepsForWire;     // Всего шагов для провода за оборот
};

#endif // TYPES_H

