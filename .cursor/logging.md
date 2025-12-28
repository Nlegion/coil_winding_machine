# Правила логирования для Hexapod проекта

## Общие принципы

### 1. Использование Core::Logger
- **ВСЕГДА** используйте `Core::Logger::log()` вместо `Serial.print()` или `printf()`
- Логирование должно быть структурированным и информативным
- Используйте правильные уровни логирования

### 2. Уровни логирования

```cpp
Core::Logger::DEBUG   // Детальная отладочная информация (только для разработки)
Core::Logger::INFO    // Обычные информационные сообщения
Core::Logger::WARNING // Предупреждения о потенциальных проблемах
Core::Logger::ERROR   // Критические ошибки
```

**Правила выбора уровня:**
- `DEBUG`: Детальная информация о внутренних процессах (только в режиме отладки)
- `INFO`: Нормальная работа системы, важные события (инициализация, команды, изменения состояния)
- `WARNING`: Проблемы, которые не останавливают работу (небезопасные значения, таймауты, fallback режимы)
- `ERROR`: Критические ошибки, которые требуют внимания (сбой инициализации, emergency stop, критические состояния)

### 3. Формат логов

**Структура:**
```
[HH:MM:SS.mmm] [LEVEL] Message with context
```

**Примеры правильного логирования:**

```cpp
// ✅ ПРАВИЛЬНО: Информация о команде
Core::Logger::log(Core::Logger::INFO, "Command received: %s", command);

// ✅ ПРАВИЛЬНО: Инициализация с контекстом
Core::Logger::log(Core::Logger::INFO, "GaitService initialized");

// ✅ ПРАВИЛЬНО: Предупреждение с деталями
Core::Logger::log(Core::Logger::WARNING, 
    "Leg %d: Angles out of safe range", leg->getId());

// ✅ ПРАВИЛЬНО: Ошибка с контекстом
Core::Logger::log(Core::Logger::ERROR, 
    "Cannot start movement: Safety check failed");

// ✅ ПРАВИЛЬНО: Критическое состояние батареи
Core::Logger::log(Core::Logger::ERROR, 
    "🔋 CRITICAL BATTERY: %.2fV", status.voltage);
```

### 4. Что логировать

**ОБЯЗАТЕЛЬНО логировать:**
- ✅ Инициализацию всех сервисов и компонентов
- ✅ Получение команд от пользователя
- ✅ Изменения состояния (start/stop movement, phase changes)
- ✅ Ошибки безопасности (unsafe pulses, angles, battery)
- ✅ Критические события (emergency stop, critical battery)
- ✅ Ошибки инициализации hardware
- ✅ Fallback режимы (WiFi → AP mode)

**НЕ логировать:**
- ❌ Каждый вызов update() в цикле (слишком часто)
- ❌ Детали каждого шага походки (используйте DEBUG уровень)
- ❌ Успешные операции без контекста (только если важно)

### 5. Контекст в логах

**Всегда включайте контекст:**
- Номер ноги/сустава при проблемах
- Значения параметров (voltage, pulse, angle)
- Идентификаторы команд
- Состояния (direction, phase, step)

```cpp
// ✅ ХОРОШО: С контекстом
Core::Logger::log(Core::Logger::WARNING, 
    "Leg %d: Pulses out of valid range (coxa=%d, femur=%d, tibia=%d)", 
    leg->getId(), pulses.coxa, pulses.femur, pulses.tibia);

// ❌ ПЛОХО: Без контекста
Core::Logger::log(Core::Logger::WARNING, "Invalid pulses");
```

### 6. Эмодзи в логах (опционально)

Можно использовать эмодзи для визуального выделения:
- 🦾 - Gait task
- 🌐 - Web/Network
- 🔋 - Battery
- 🚨 - Emergency
- ✅ - Success
- ❌ - Error
- 🔍 - Diagnostic/Test

**Использовать умеренно**, только для важных событий.

### 7. Производительность

- **НЕ** логируйте в tight loops без проверки
- Используйте условную компиляцию для DEBUG логов:
  ```cpp
  #ifdef DEBUG_MODE
      Core::Logger::log(Core::Logger::DEBUG, "Detailed info");
  #endif
  ```

### 8. Безопасность

- **НЕ** логируйте пароли, токены, чувствительные данные
- **НЕ** логируйте в критических секциях (может вызвать задержки)

### 9. Примеры по слоям архитектуры

**Domain Layer:**
```cpp
// Только важные события доменной логики
Core::Logger::log(Core::Logger::WARNING, 
    "Leg %d: Angles out of safe range", leg->getId());
```

**Application Layer:**
```cpp
// Команды и Use Cases
Core::Logger::log(Core::Logger::INFO, "Command received: %s", command);
Core::Logger::log(Core::Logger::INFO, "Movement started: direction=%d", (int)direction);
```

**Infrastructure Layer:**
```cpp
// Hardware взаимодействие
Core::Logger::log(Core::Logger::INFO, "Servo controller initialized successfully");
Core::Logger::log(Core::Logger::ERROR, "Failed to initialize servo controller");
```

**Presentation Layer:**
```cpp
// Сетевые события
Core::Logger::log(Core::Logger::INFO, "✅ WiFi connected! IP: %s", 
    WiFi.localIP().toString().c_str());
```

### 10. Анти-паттерны

```cpp
// ❌ ПЛОХО: Использование Serial напрямую
Serial.println("Something happened");

// ❌ ПЛОХО: Логирование без контекста
Core::Logger::log(Core::Logger::INFO, "Error");

// ❌ ПЛОХО: Слишком частые логи в цикле
void update() {
    Core::Logger::log(Core::Logger::INFO, "Update called"); // Каждый раз!
}

// ❌ ПЛОХО: Неправильный уровень
Core::Logger::log(Core::Logger::ERROR, "Normal operation"); // Это INFO!

// ✅ ХОРОШО: Правильное использование
if (someError) {
    Core::Logger::log(Core::Logger::ERROR, 
        "Operation failed: %s (code: %d)", errorMsg, errorCode);
}
```

## Резюме

1. **Всегда** используйте `Core::Logger::log()` с правильным уровнем
2. **Всегда** включайте контекст (ID, значения, состояние)
3. **Логируйте** важные события, но не каждый вызов функции
4. **Используйте** правильный уровень для типа сообщения
5. **Избегайте** логирования в tight loops без проверки

