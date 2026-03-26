#pragma once
#include "../utils.h"
#include "../configservice.h"

#define IDC_MSG_WELCOME_EDIT   3401
#define IDC_MSG_PLANS_EDIT     3402
#define IDC_MSG_SETUP_EDIT     3403
#define IDC_MSG_PAYINSTR_EDIT  3404
#define IDC_MSG_ADMINNOTIFY_EDIT 3405
#define IDC_MSG_CHECKRECEIVED_EDIT 3406
#define IDC_MSG_CONFIRMED_EDIT 3407
#define IDC_MSG_REJECTED_EDIT  3408
#define IDC_MSG_SAVE           3409

namespace MessagesPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc, ConfigService* config);
    void loadValues(HWND hwnd, ConfigService* config);
}
