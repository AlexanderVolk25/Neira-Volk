#pragma once
#include "../utils.h"
#include "../configservice.h"
#include "../botengine.h"

#define IDC_DASH_STATUS_DOT    3001
#define IDC_DASH_STATUS_LABEL  3002
#define IDC_DASH_STARTSTOP     3003
#define IDC_DASH_CLEAR         3004
#define IDC_DASH_LOG           3005

namespace DashboardPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc,
                ConfigService* config, BotEngine* engine);
    void appendLog(HWND hwnd, const std::string& text);
}
