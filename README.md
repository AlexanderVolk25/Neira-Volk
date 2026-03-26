# Neira Bot Panel

Панель управления Telegram-ботом для продажи подписок.  
Собирается как **нативное Windows-приложение** с помощью **Visual Studio 2022** — без Qt.

---

## Стек технологий

| Компонент | Используется |
|-----------|-------------|
| UI        | Win32 API (Common Controls) |
| HTTP      | WinHTTP (Windows SDK, встроен) |
| База данных | SQLite 3 через vcpkg |
| JSON      | nlohmann/json через vcpkg |
| Сборка    | Visual Studio 2022 (`.sln` + `.vcxproj`) |

---

## Требования

- **Visual Studio 2022** с компонентом «Разработка классических приложений на C++»
- **vcpkg** (менеджер пакетов C++) — для получения sqlite3 и nlohmann-json

---

## Установка зависимостей через vcpkg

```cmd
# 1. Установить vcpkg (если ещё не установлен)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# 2. Интегрировать с Visual Studio (один раз)
C:\vcpkg\vcpkg integrate install

# 3. Установить зависимости (или использовать автоустановку через vcpkg.json)
C:\vcpkg\vcpkg install nlohmann-json:x64-windows sqlite3:x64-windows

# 4. Установить переменную окружения (если нужно)
set VCPKG_ROOT=C:\vcpkg
```

Либо откройте решение в Visual Studio — оно автоматически обнаружит `vcpkg.json` и предложит установить зависимости.

---

## Сборка

1. Откройте `NeiraBotPanel.sln` в Visual Studio 2022
2. Выберите конфигурацию **Release | x64**
3. Соберите: **Build → Build Solution** (или `Ctrl+Shift+B`)
4. Готовый EXE находится в `build\Release\NeiraBotPanel.exe`

---

## Функционал

### Вкладки панели управления

| Вкладка    | Описание |
|------------|----------|
| Dashboard  | Запуск/остановка бота, просмотр лога в реальном времени |
| Settings   | Токен бота, ID администратора, поддержка, канал |
| Plans      | Тарифные планы (название, цена, срок) |
| Payments   | Ссылка для ручной оплаты; настройка автопровайдеров |
| Messages   | Шаблоны всех сообщений бота |
| Channel    | Публикация объявлений в Telegram-канал |
| Orders     | Таблица заказов с фильтрацией по статусу |

### Логика бота

- **Long-polling** Telegram API (таймаут 25 с)
- **Машина состояний** пользователя: `Idle → SelectingPlan → SelectingPaymentMethod → WaitingReceipt → Idle`
- **Поток администратора**: чек → уведомление → подтвердить/отклонить → отправить данные доступа
- **SQLite** база данных: `bot.db` (заказы и пользователи)
- **Конфигурация**: `config.json` рядом с EXE

---

## Структура проекта

```
NeiraBotPanel.sln          Visual Studio Solution
NeiraBotPanel.vcxproj      Visual Studio Project
NeiraBotPanel.vcxproj.filters
vcpkg.json                 Зависимости (nlohmann-json, sqlite3)
src/
├── main.cpp               WinMain — точка входа
├── mainwindow.h/.cpp      Главное Win32-окно с боковой навигацией
├── utils.h                Утилиты (UTF-8 ↔ WCHAR, создание контролов)
├── configservice.h/.cpp   Чтение/запись config.json (nlohmann/json)
├── orderservice.h/.cpp    SQLite3: заказы и пользователи
├── telegramapiclient.h/.cpp  WinHTTP: Telegram Bot API
├── botengine.h/.cpp       Логика бота (std::thread, машина состояний)
├── adminworkflow.h/.cpp   Состояние подтверждения заказов администратором
└── pages/
    ├── dashboardpage.h/.cpp
    ├── settingspage.h/.cpp
    ├── planspage.h/.cpp
    ├── paymentspage.h/.cpp
    ├── messagespage.h/.cpp
    ├── channelpage.h/.cpp
    └── orderspage.h/.cpp
```
