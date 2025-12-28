#ifndef CONFIG_H
#define CONFIG_H

// ==================== КОНСТАНТЫ ====================
#define DEBUG_MODE true  // Включить/выключить отладочные сообщения

// Параметры концевиков
#define ENDSTOP_DEBOUNCE_MS 50      // Время подавления дребезга (мс)
#define ENDSTOP_WARNING_DIST_MM 2.0 // Расстояние предупреждения (мм)

// Таймауты
#define HOMING_TIMEOUT_MS 240000    // Таймаут калибровки (240 сек = 4 минуты)
#define MOVE_TIMEOUT_MS 30000      // Таймаут движения (30 сек)

// Параметры WiFi
#define WIFI_CHECK_INTERVAL_MS 5000 // Интервал проверки WiFi (5 сек)
#define WIFI_RECONNECT_ATTEMPTS 3   // Попыток переподключения
#define WIFI_TIMEOUT 20             // Таймаут подключения к WiFi (секунды)

// Параметры памяти
#define MIN_FREE_HEAP 5000          // Минимальная свободная память (байт)

// ==================== НАСТРОЙКА ПИНОВ ====================
// Двигатель 1 (Вращение шпульки)
#define MOTOR_ROT_STEP    D1 // GPIO5
#define MOTOR_ROT_DIR     D2 // GPIO4

// Двигатель 2 (Перемещение суппорта)
#define MOTOR_LIN_STEP    D3 // GPIO0
#define MOTOR_LIN_DIR     D4 // GPIO2
#define MOTOR_ENABLE      D7 // GPIO13 (общий для двух драйверов)

// Концевые выключатели (SS-5GL, нормально разомкнутые - NO)
#define ENDSTOP_HOME      D5 // GPIO14 - Левый (начало)
#define ENDSTOP_END       D6 // GPIO12 - Правый (конец)

// ==================== ПАРАМЕТРЫ ДВИГАТЕЛЕЙ ====================
extern const int STEPS_PER_REV;     // Шагов на оборот для NEMA 17
extern const int MICROSTEPS;         // Драйвер настроен на 1/16 микрошага
extern const float SCREW_PITCH;     // Шаг ходового винта (мм за оборот)
extern const int TOTAL_STEPS_PER_REV; // 3200
extern const float STEPS_PER_MM; // 1600 шагов/мм

// Константа для homing
extern const int HOME_BACKOFF_STEPS_TARGET;

#endif // CONFIG_H

