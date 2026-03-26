#pragma once
#include "../utils.h"
#include "../configservice.h"
#include "../botengine.h"

#define IDC_CHAN_POST_EDIT   3501
#define IDC_CHAN_PUBLISH     3502
#define IDC_CHAN_STATUS      3503

namespace ChannelPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc,
                ConfigService* config, BotEngine* engine);
    void loadValues(HWND hwnd, ConfigService* config);
}
