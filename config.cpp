#include "config.h"

// ==================== ПАРАМЕТРЫ ДВИГАТЕЛЕЙ ====================
const int STEPS_PER_REV = 200;     // Шагов на оборот для NEMA 17
const int MICROSTEPS = 16;         // Драйвер настроен на 1/16 микрошага
const float SCREW_PITCH = 2.0;     // Шаг ходового винта (мм за оборот)
const int TOTAL_STEPS_PER_REV = STEPS_PER_REV * MICROSTEPS; // 3200
const float STEPS_PER_MM = TOTAL_STEPS_PER_REV / SCREW_PITCH; // 1600 шагов/мм

// Константа для homing
const int HOME_BACKOFF_STEPS_TARGET = 500;

