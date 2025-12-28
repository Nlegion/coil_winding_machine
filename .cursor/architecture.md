# Правила архитектуры для Hexapod проекта

## Общие принципы

Проект использует **Clean Architecture** с разделением на слои. Все изменения должны соответствовать этой архитектуре.

## Структура слоёв

```
┌─────────────────────────────────────────┐
│   PRESENTATION LAYER (Interface)        │  ← WebServer, WebSocket, CLI
├─────────────────────────────────────────┤
│   APPLICATION LAYER (Use Cases)         │  ← Movement, Gestures, Control
├─────────────────────────────────────────┤
│   DOMAIN LAYER (Business Logic)         │  ← Gait, Kinematics, Safety
├─────────────────────────────────────────┤
│   INFRASTRUCTURE LAYER (Hardware)       │  ← Servo, WiFi, Battery, Serial
└─────────────────────────────────────────┘
```

## Правило зависимостей (Dependency Rule)

**КРИТИЧЕСКИЕ ПРАВИЛА:**

1. **Зависимости направлены ВНУТРЬ** (к Domain Layer)
2. **Domain Layer НЕ зависит** от Infrastructure или Presentation
3. **Application Layer** зависит только от Domain
4. **Infrastructure Layer** реализует интерфейсы из Domain
5. **Presentation Layer** зависит от Application

```
Presentation → Application → Domain ← Infrastructure
```

## Правила по слоям

### 1. Core Layer (`src/core/`)

**Назначение:** Общие типы, конфигурация, утилиты

**Содержит:**
- `Types.h` - общие типы и enums
- `Config.h` - константы конфигурации
- `Logger.h` - система логирования

**Правила:**
- ✅ НЕ зависит от других слоёв
- ✅ Только константы, типы, утилиты
- ✅ НЕ содержит бизнес-логику
- ✅ НЕ содержит зависимости от hardware

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Только константы
namespace Core {
namespace Config {
    constexpr int MIN_PULSE = 1000;
    constexpr float MAX_COXA_ANGLE = 45.0f;
}
}
```

### 2. Domain Layer (`src/domain/`)

**Назначение:** Бизнес-логика, независимая от hardware

**Структура:**
```
domain/
├── entities/          # Сущности (Leg, Body)
├── services/          # Доменные сервисы (Gait, Kinematics, Safety)
└── repositories/      # Интерфейсы репозиториев (IServoRepository)
```

**Правила:**
- ✅ **НЕ зависит** от Infrastructure или Presentation
- ✅ Содержит только бизнес-логику
- ✅ Использует интерфейсы (IServoRepository, ISafetyService)
- ✅ НЕ содержит вызовов hardware напрямую
- ✅ Может зависеть только от Core

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Domain Service с интерфейсами
class GaitService : public IGaitService {
    std::shared_ptr<IServoRepository> servos_;  // Интерфейс!
    std::shared_ptr<ISafetyService> safety_;     // Интерфейс!
    
    void executeGaitStep() {
        // Бизнес-логика БЕЗ вызовов hardware
        servos_->setLegPosition(...);  // Через интерфейс
    }
};

// ❌ ПЛОХО: Прямой вызов hardware
class GaitService {
    void executeGaitStep() {
        Serial1.print("#1P1500T200\r\n");  // НЕТ! Это Infrastructure
    }
};
```

### 3. Application Layer (`src/application/`)

**Назначение:** Use Cases и координация

**Структура:**
```
application/
├── usecases/          # Use Cases (MoveForward, Turn, etc.)
├── dto/              # Data Transfer Objects
└── RobotController.h # Координатор Use Cases
```

**Правила:**
- ✅ Зависит только от Domain и Core
- ✅ Содержит Use Cases (один Use Case = одна бизнес-операция)
- ✅ Координирует вызовы Domain сервисов
- ✅ НЕ содержит бизнес-логику (логика в Domain)
- ✅ НЕ зависит от Infrastructure или Presentation

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Use Case координирует Domain
class MoveForwardUseCase {
    std::shared_ptr<Domain::IGaitService> gait_;
    std::shared_ptr<Domain::ISafetyService> safety_;
    
    void execute(float speed) {
        if (!safety_->canMove()) return;  // Проверка через Domain
        gait_->startMovement(...);         // Вызов Domain
    }
};

// ❌ ПЛОХО: Use Case с бизнес-логикой
class MoveForwardUseCase {
    void execute() {
        // Бизнес-логика должна быть в Domain!
        calculateTrajectory();  // НЕТ! Это Domain
    }
};
```

### 4. Infrastructure Layer (`src/infrastructure/`)

**Назначение:** Реализации для hardware и внешних систем

**Структура:**
```
infrastructure/
├── hardware/         # ServoRepository, BatteryMonitor
└── network/          # WiFi, WebSocket (если нужно)
```

**Правила:**
- ✅ Реализует интерфейсы из Domain
- ✅ Содержит все вызовы hardware
- ✅ Может зависеть от Core
- ✅ НЕ содержит бизнес-логику
- ✅ Изолирует hardware детали

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Реализация интерфейса
class ServoRepository : public Domain::IServoRepository {
    HardwareSerial& serial_;
    
    void setLegPosition(LegID leg, const ServoPulses& pulses, int time) override {
        // Hardware детали здесь
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "#%dP%dT%d\r\n", channel, pulse, time);
        serial_.write(cmd);
    }
};
```

### 5. Presentation Layer (`src/presentation/`)

**Назначение:** Интерфейсы пользователя (Web, CLI)

**Структура:**
```
presentation/
├── web/              # WebController, WebSocketController
└── cli/              # SerialController (если нужно)
```

**Правила:**
- ✅ Зависит только от Application и Core
- ✅ Преобразует пользовательский ввод в команды Application
- ✅ Преобразует ответы Application в формат для пользователя
- ✅ НЕ содержит бизнес-логику
- ✅ НЕ зависит от Domain напрямую (только через Application)

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Presentation вызывает Application
class WebController {
    std::shared_ptr<Application::RobotController> robot_;
    
    void handleCommand(const String& cmd) {
        robot_->handleCommand(cmd.c_str());  // Через Application
    }
};

// ❌ ПЛОХО: Presentation вызывает Domain напрямую
class WebController {
    std::shared_ptr<Domain::GaitService> gait_;  // НЕТ! Через Application
};
```

## Dependency Injection

**Всегда используйте DI Container** для управления зависимостями.

**Правила:**
- ✅ Все зависимости инжектируются через конструктор
- ✅ Используйте `std::shared_ptr` для зависимостей
- ✅ Регистрируйте все сервисы в `DI::Container`
- ✅ НЕ создавайте зависимости внутри классов (кроме Infrastructure)

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Dependency Injection
class GaitService {
    std::shared_ptr<IKinematicsService> kinematics_;
    std::shared_ptr<ISafetyService> safety_;
    
public:
    GaitService(
        std::shared_ptr<IKinematicsService> kinematics,
        std::shared_ptr<ISafetyService> safety
    ) : kinematics_(kinematics), safety_(safety) {}
};

// ❌ ПЛОХО: Создание зависимостей внутри
class GaitService {
    std::shared_ptr<KinematicsService> kinematics_;
    
public:
    GaitService() {
        kinematics_ = std::make_shared<KinematicsService>();  // НЕТ!
    }
};
```

## Namespaces

**Используйте namespaces по слоям:**
- `Core::` - Core Layer
- `Domain::` - Domain Layer
- `Application::` - Application Layer
- `Infrastructure::` - Infrastructure Layer
- `Presentation::` - Presentation Layer
- `DI::` - Dependency Injection

## FreeRTOS Tasks

**Правила для FreeRTOS:**
- ✅ Tasks создаются только в `hexapod.ino` (main)
- ✅ Tasks получают контроллеры через DI Container
- ✅ Каждый task отвечает за одну область (gait, web, battery)
- ✅ Используйте `vTaskDelay()` вместо `delay()`
- ✅ Используйте приоритеты задач правильно

**Пример:**
```cpp
// ✅ ПРАВИЛЬНО: Task получает контроллер из DI
void gaitTask(void* parameter) {
    auto robotController = container.getRobotController();
    while (true) {
        robotController->update(millis());
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

## Анти-паттерны

### ❌ Нарушение Dependency Rule
```cpp
// Domain зависит от Infrastructure
#include "../infrastructure/hardware/ServoRepository.h"  // НЕТ!
```

### ❌ Глобальные переменные
```cpp
// Вместо этого
LegController hexapod;  // Глобальная переменная

// Используйте DI Container
container.getRobotController();
```

### ❌ Бизнес-логика в Application
```cpp
// Бизнес-логика должна быть в Domain
class MoveForwardUseCase {
    void execute() {
        calculateIK();  // НЕТ! Это Domain
    }
};
```

### ❌ Прямые вызовы hardware из Domain
```cpp
// Domain не должен знать о hardware
class GaitService {
    void execute() {
        Serial1.print("#1P1500\r\n");  // НЕТ! Это Infrastructure
    }
};
```

## Резюме

1. **Соблюдайте Dependency Rule** - зависимости направлены внутрь
2. **Используйте интерфейсы** - Domain определяет интерфейсы, Infrastructure реализует
3. **Dependency Injection** - все зависимости через конструктор
4. **Разделение ответственности** - каждый слой отвечает за свою область
5. **Тестируемость** - Domain и Application можно тестировать без hardware

