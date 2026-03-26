#pragma once
#include "../utils.h"
#include "../configservice.h"

#define IDC_SET_TOKEN_EDIT    3101
#define IDC_SET_ADMIN_EDIT    3102
#define IDC_SET_SUPPORT_EDIT  3103
#define IDC_SET_CHANNEL_EDIT  3104
#define IDC_SET_AUTOPAY_CHK   3105
#define IDC_SET_SAVE          3106

namespace SettingsPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc, ConfigService* config);
    void loadValues(HWND hwnd, ConfigService* config);
}
