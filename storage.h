#ifndef STORAGE_H
#define STORAGE_H

#include "types.h"

// Инициализация EEPROM
void initStorage();

// Работа с настройками
void saveSettings();
void loadSettings();
bool validateSettings();

// Работа с позицией
void savePosition();
void loadPosition();

// Глобальная переменная настроек
extern CoilSettings settings;

#endif // STORAGE_H

