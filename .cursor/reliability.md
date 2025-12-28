# Правила надежности для Hexapod проекта

## Общие принципы

Робот - это система безопасности. Все изменения должны повышать надежность и безопасность системы.

## 1. Проверки безопасности (Safety Checks)

### Обязательные проверки перед действиями

**ВСЕГДА проверяйте:**
- ✅ Безопасность pulse значений перед отправкой на сервоприводы
- ✅ Безопасность углов суставов
- ✅ Состояние батареи перед движением
- ✅ Валидность входных параметров
- ✅ Состояние системы перед критическими операциями

**Примеры:**

```cpp
// ✅ ПРАВИЛЬНО: Проверка перед движением
void startMovement(Direction dir) {
    if (!safety_->canMove()) {
        Core::Logger::log(Core::Logger::ERROR, 
            "Cannot start movement: Safety check failed");
        return;  // Безопасный выход
    }
    // Продолжаем только если безопасно
}

// ✅ ПРАВИЛЬНО: Проверка pulse перед отправкой
void setServoPosition(int channel, int pulse) {
    pulse = safety_->constrainPulse(pulse);  // Ограничение
    if (!safety_->isPulseSafe(pulse)) {
        Core::Logger::log(Core::Logger::WARNING, 
            "Unsafe pulse %d for channel %d", pulse, channel);
        return;
    }
    // Отправка только безопасных значений
}

// ✅ ПРАВИЛЬНО: Проверка углов
bool isAngleSafe(float angle, JointID joint) {
    switch (joint) {
        case COXA:
            return abs(angle) <= Core::Config::MAX_COXA_ANGLE;
        case FEMUR:
            return abs(angle) <= Core::Config::MAX_FEMUR_ANGLE;
        case TIBIA:
            return abs(angle) <= Core::Config::MAX_TIBIA_ANGLE;
        default:
            return false;  // Неизвестный сустав = небезопасно
    }
}
```

### Использование SafetyService

**ВСЕГДА используйте SafetyService для проверок:**

```cpp
// ✅ ПРАВИЛЬНО: Через SafetyService
pulse = safety_->constrainPulse(pulse);
if (!safety_->isPulseSafe(pulse)) {
    // Обработка ошибки
}

// ❌ ПЛОХО: Прямая проверка без SafetyService
if (pulse < 1000 || pulse > 2000) {
    // Дублирование логики, может устареть
}
```

## 2. Обработка ошибок

### Принципы обработки ошибок

1. **Fail-Safe** - при ошибке система должна перейти в безопасное состояние
2. **Логирование** - все ошибки должны логироваться
3. **Graceful Degradation** - система должна продолжать работать при некритических ошибках
4. **Emergency Stop** - критичные ошибки должны останавливать движение

**Примеры:**

```cpp
// ✅ ПРАВИЛЬНО: Fail-Safe обработка
bool initializeServoController() {
    if (!servos_->initialize()) {
        Core::Logger::log(Core::Logger::ERROR, 
            "Failed to initialize servo controller");
        // Переход в безопасное состояние
        emergencyStop();
        return false;
    }
    return true;
}

// ✅ ПРАВИЛЬНО: Graceful Degradation
void updateBattery() {
    BatteryStatus status = battery_->getStatus();
    if (status.isCritical) {
        Core::Logger::log(Core::Logger::ERROR, 
            "🔋 CRITICAL BATTERY: %.2fV", status.voltage);
        // Останавливаем движение, но система продолжает работать
        gait_->stopMovement();
        safety_->setBatteryStatus(status);
    }
}

// ✅ ПРАВИЛЬНО: Валидация входных данных
void execute(float speed) {
    // Проверка входных параметров
    if (speed < 0.0f || speed > 1.0f) {
        Core::Logger::log(Core::Logger::WARNING, 
            "Invalid speed: %.2f, constraining to [0.0, 1.0]", speed);
        speed = constrainFloat(speed, 0.0f, 1.0f);
    }
    // Продолжаем с валидными данными
}
```

### Emergency Stop

**Критичные ситуации требуют немедленной остановки:**

```cpp
// ✅ ПРАВИЛЬНО: Emergency Stop при критических ошибках
void emergencyStop() {
    Core::Logger::log(Core::Logger::ERROR, "🚨 EMERGENCY STOP!");
    
    // 1. Останавливаем движение
    gait_->stopMovement();
    
    // 2. Переводим все сервоприводы в нейтральное положение
    for (int leg = 0; leg < TOTAL_LEGS; leg++) {
        servos_->setLegPosition(leg, neutralPulses, 200);
    }
    
    // 3. Логируем событие
    Core::Logger::log(Core::Logger::ERROR, 
        "Emergency stop completed - all servos in neutral");
}
```

## 3. Валидация входных данных

### Проверка всех входных параметров

**ВСЕГДА валидируйте:**
- Параметры функций
- Команды от пользователя
- Данные из сети
- Конфигурационные значения

```cpp
// ✅ ПРАВИЛЬНО: Валидация параметров
void setSpeed(float speed) {
    // Ограничение значения
    speed_ = constrainFloat(speed, 0.0f, 1.0f);
    Core::Logger::log(Core::Logger::INFO, "Speed set to: %.2f", speed_);
}

// ✅ ПРАВИЛЬНО: Валидация команд
void handleCommand(const char* command) {
    if (command == nullptr || strlen(command) == 0) {
        Core::Logger::log(Core::Logger::WARNING, "Empty command received");
        return;
    }
    // Обработка команды
}

// ✅ ПРАВИЛЬНО: Валидация индексов
void testLeg(int legId) {
    if (legId < 0 || legId >= TOTAL_LEGS) {
        Core::Logger::log(Core::Logger::WARNING, 
            "Invalid leg ID: %d (must be 0-%d)", legId, TOTAL_LEGS - 1);
        return;
    }
    // Безопасная обработка
}
```

## 4. Защита от переполнения и переполнения буферов

### Безопасная работа с буферами

```cpp
// ✅ ПРАВИЛЬНО: Безопасное форматирование
char buffer[128];
snprintf(buffer, sizeof(buffer), 
    "BATTERY:%.2f:%.1f:%d:%d",
    status.voltage,
    status.percentage,
    status.isLow ? 1 : 0,
    status.isCritical ? 1 : 0
);
// snprintf гарантирует, что не будет переполнения

// ❌ ПЛОХО: Небезопасное форматирование
sprintf(buffer, "...");  // Может переполнить буфер!
```

### Проверка границ массивов

```cpp
// ✅ ПРАВИЛЬНО: Проверка границ
int getServoChannel(LegID leg, JointID joint) {
    if (leg < 0 || leg >= TOTAL_LEGS) return -1;
    if (joint < 0 || joint >= NUM_JOINTS) return -1;
    return Core::Config::LEG_SERVO_MAP[leg][joint];
}

// ❌ ПЛОХО: Без проверки
int getServoChannel(LegID leg, JointID joint) {
    return LEG_SERVO_MAP[leg][joint];  // Может выйти за границы!
}
```

## 5. Защита от состояний гонки (Race Conditions)

### FreeRTOS и многопоточность

**Правила для FreeRTOS tasks:**
- ✅ Используйте mutex/semaphore для разделяемых ресурсов
- ✅ Избегайте глобальных переменных без защиты
- ✅ Используйте atomic операции где возможно
- ✅ Документируйте thread-safety

```cpp
// ✅ ПРАВИЛЬНО: Защита разделяемых ресурсов
SemaphoreHandle_t batteryMutex = xSemaphoreCreateMutex();

void batteryTask(void* parameter) {
    while (true) {
        if (xSemaphoreTake(batteryMutex, portMAX_DELAY)) {
            BatteryStatus status = battery_->getStatus();
            safety_->setBatteryStatus(status);  // Защищённый доступ
            xSemaphoreGive(batteryMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

## 6. Timeout и таймауты

### Защита от зависаний

**ВСЕГДА используйте таймауты для:**
- WiFi подключения
- Инициализации hardware
- Операций, которые могут зависнуть

```cpp
// ✅ ПРАВИЛЬНО: Timeout для WiFi
void setupWiFi() {
    WiFi.begin(SSID, PASSWORD);
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startTime < Core::Config::WIFI_TIMEOUT) {
        delay(500);
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Core::Logger::log(Core::Logger::WARNING, 
            "WiFi timeout, starting AP mode");
        // Fallback режим
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Hexapod_Config", "12345678");
    }
}
```

## 7. Проверка состояния системы

### Проверки перед критическими операциями

```cpp
// ✅ ПРАВИЛЬНО: Проверка состояния перед движением
void startMovement(Direction dir) {
    // 1. Проверка батареи
    if (!safety_->canMove()) {
        Core::Logger::log(Core::Logger::ERROR, 
            "Cannot move: Safety check failed");
        return;
    }
    
    // 2. Проверка инициализации
    if (!container.isInitialized()) {
        Core::Logger::log(Core::Logger::ERROR, 
            "System not initialized");
        return;
    }
    
    // 3. Останавливаем предыдущее движение
    if (gait_->isMoving()) {
        gait_->stopMovement();
    }
    
    // 4. Начинаем движение
    gait_->startMovement(dir);
}
```

## 8. Обработка исключений (для C++)

### Безопасная обработка ошибок

```cpp
// ✅ ПРАВИЛЬНО: Try-catch для критических операций
void performCriticalOperation() {
    try {
        // Критическая операция
        servos_->setLegPosition(leg, pulses, time);
    } catch (const std::exception& e) {
        Core::Logger::log(Core::Logger::ERROR, 
            "Critical operation failed: %s", e.what());
        emergencyStop();
    }
}
```

**Примечание:** На ESP32 исключения могут быть ограничены, используйте проверки возвращаемых значений.

## 9. Мониторинг и диагностика

### Регулярные проверки состояния

```cpp
// ✅ ПРАВИЛЬНО: Регулярный мониторинг батареи
void batteryTask(void* parameter) {
    while (true) {
        batteryMonitor->update();
        BatteryStatus status = batteryMonitor->getStatus();
        
        // Обновляем Safety Service
        safety_->setBatteryStatus(status);
        
        // Проверяем критический уровень
        if (status.isCritical) {
            Core::Logger::log(Core::Logger::ERROR, 
                "🔋 CRITICAL BATTERY: %.2fV", status.voltage);
            gait_->stopMovement();  // Останавливаем движение
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

## 10. Анти-паттерны надежности

### ❌ Игнорирование ошибок
```cpp
// ПЛОХО
servos_->setLegPosition(leg, pulses, time);  // Игнорируем результат
```

### ❌ Небезопасные операции
```cpp
// ПЛОХО
int pulse = calculatePulse();  // Может быть вне безопасного диапазона
Serial1.print("#1P"); Serial1.print(pulse);  // Без проверки!
```

### ❌ Отсутствие проверок
```cpp
// ПЛОХО
void setSpeed(float speed) {
    speed_ = speed;  // Без валидации!
}
```

### ❌ Блокирующие операции
```cpp
// ПЛОХО
void update() {
    delay(1000);  // Блокирует всю систему!
}

// ХОРОШО
void update() {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Неблокирующая задержка
}
```

## Резюме

1. **Всегда проверяйте безопасность** перед действиями
2. **Валидируйте входные данные** на всех уровнях
3. **Используйте SafetyService** для всех проверок безопасности
4. **Обрабатывайте ошибки gracefully** с логированием
5. **Используйте таймауты** для операций, которые могут зависнуть
6. **Защищайте разделяемые ресурсы** в многопоточном коде
7. **Реализуйте fail-safe** механизмы для критических ошибок
8. **Мониторьте состояние системы** регулярно

**Помните:** Робот - это система безопасности. Надежность важнее производительности.

