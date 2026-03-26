# Neira Bot Panel

A Telegram bot control panel built with Qt6 (C++), targeting Windows EXE.

## Prerequisites

- Qt 6.4+ (with Widgets, Network, Sql modules)
- CMake 3.21+
- Visual Studio 2022 with MSVC (x64)

## Build Steps

1. Clone the repository:
   ```
   git clone <repo-url>
   cd Neira-Volk
   ```

2. Configure and build:
   ```
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

3. Find the executable at `build/Release/NeiraBotPanel.exe`.

## Configuration

1. Launch `NeiraBotPanel.exe`.
2. Open the **Settings** tab.
3. Enter your **Bot Token** (from [@BotFather](https://t.me/BotFather)).
4. Enter your **Admin Telegram ID** (numeric).
5. (Optional) Set a support @username and channel username/ID.
6. Click **Save**, then click **Start** on the Dashboard.

## Features

- **Dashboard**: Start/stop bot, live log viewer.
- **Settings**: Bot token, admin ID, support username, channel, auto-payment toggle.
- **Plans**: Manage subscription plans (name, price, duration).
- **Payments**: Configure manual payment link; auto-payment provider skeleton (T-Bank, Sber, Other).
- **Messages**: Edit all bot message templates.
- **Channel**: Publish posts to your Telegram channel.
- **Orders**: View and filter all orders stored in SQLite.

## Payment Modes

### Manual Payment
One universal payment link (e.g. Tinkoff) where the user selects the amount.
After paying, the user must send a receipt (photo or PDF).
The admin reviews the receipt and confirms or rejects the order.

### Auto Payment (skeleton)
Bank selection submenu (T-Bank / Sber / Other) is a framework — no real API integration yet.
Enable via the **Show Auto Payment** checkbox in Settings to expose the UI.
Each provider has configurable API Key / Terminal Key / Password fields for future integration.

## Channel Integration

Add the bot as an admin of your channel and set the channel username/ID in Settings.

## Database

An SQLite database (`bot.db`) is created automatically next to the executable on first run.

## How to Get a Bot Token

1. Open Telegram and start a chat with [@BotFather](https://t.me/BotFather).
2. Send `/newbot` and follow the prompts.
3. Copy the token provided and paste it into the Settings tab.
