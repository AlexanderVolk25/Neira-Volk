#pragma once
#include "../utils.h"
#include "../configservice.h"

#define IDC_PLANS_LIST   3201
#define IDC_PLANS_ADD    3202
#define IDC_PLANS_REMOVE 3203
#define IDC_PLANS_SAVE   3204

namespace PlansPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc, ConfigService* config);
    void loadValues(HWND hwnd, ConfigService* config);
}
