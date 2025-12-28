#include "storage.h"
#include "config.h"
#include "logger.h"
#include "position.h"
#include <EEPROM.h>

// Глобальная переменная настроек
CoilSettings settings;

void initStorage() {
  EEPROM.begin(4096);
  loadSettings();
  loadPosition();
}

void saveSettings() {
  EEPROM.put(0, settings);
  EEPROM.commit();
  logInfo("Настройки сохранены в EEPROM.");
}

void loadSettings() {
  EEPROM.get(0, settings);
  
  // Проверка на "пустые" значения (первый запуск)
  if (settings.wireDia < 0.01 || settings.wireDia > 5.0 ||
      settings.targetTurns < 1 || settings.targetTurns > 10000) {
    // Значения по умолчанию
    settings.wireDia = 0.5;
    settings.coreDia = 20.0;
    settings.coreLen = 50.0;
    settings.targetTurns = 200;
    settings.layers = 1;
    settings.speed = 600;
    settings.savePosition = true;
    saveSettings();
  }
}

bool validateSettings() {
  // Валидация всех параметров
  if (settings.wireDia < 0.05 || settings.wireDia > 2.0) {
    lastError = "wireDia должен быть в диапазоне 0.05-2.0 мм";
    return false;
  }
  
  if (settings.coreLen <= 0 || settings.coreLen > 500) {
    lastError = "coreLen должен быть > 0 и < 500 мм";
    return false;
  }
  
  if (settings.targetTurns < 1 || settings.targetTurns > 10000) {
    lastError = "targetTurns должен быть в диапазоне 1-10000";
    return false;
  }
  
  if (settings.speed < 100 || settings.speed > 5000) {
    lastError = "speed должен быть в диапазоне 100-5000 мкс";
    return false;
  }
  
  if (settings.coreDia <= 0 || settings.coreDia > 200) {
    lastError = "coreDia должен быть > 0 и < 200 мм";
    return false;
  }
  
  if (settings.layers < 1 || settings.layers > 100) {
    lastError = "layers должен быть в диапазоне 1-100";
    return false;
  }
  
  return true;
}

void savePosition() {
  if (settings.savePosition) {
    EEPROM.put(sizeof(CoilSettings), getCurrentPosSteps());
    EEPROM.commit();
  }
}

void loadPosition() {
  if (settings.savePosition) {
    long savedPos = 0;
    EEPROM.get(sizeof(CoilSettings), savedPos);
    if (savedPos >= 0 && savedPos < 1000000) { // Разумные пределы
      setPositionSteps(savedPos);
      logInfo("Позиция восстановлена: " + String(getCurrentPosMM()) + " мм");
    }
  }
}

