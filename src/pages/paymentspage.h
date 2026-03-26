#pragma once
#include "../utils.h"
#include "../configservice.h"

#define IDC_PAY_LINK_EDIT   3301
#define IDC_PAY_LINK_SAVE   3302
#define IDC_PAY_PROV_LIST   3303
#define IDC_PAY_PROV_SAVE   3304

namespace PaymentsPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc, ConfigService* config);
    void loadValues(HWND hwnd, ConfigService* config);
}
