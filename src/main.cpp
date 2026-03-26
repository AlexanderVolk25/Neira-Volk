#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include "configservice.h"
#include "orderservice.h"
#include "botengine.h"
#include "mainwindow.h"

#include "pages/dashboardpage.h"
#include "pages/settingspage.h"
#include "pages/planspage.h"
#include "pages/paymentspage.h"
#include "pages/messagespage.h"
#include "pages/channelpage.h"
#include "pages/orderspage.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    // Initialize Common Controls (ListView, etc.)
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    // Register all page window classes
    DashboardPage::registerClass(hInst);
    SettingsPage::registerClass(hInst);
    PlansPage::registerClass(hInst);
    PaymentsPage::registerClass(hInst);
    MessagesPage::registerClass(hInst);
    ChannelPage::registerClass(hInst);
    OrdersPage::registerClass(hInst);

    // Create services
    ConfigService config;
    config.load();

    OrderService orders;
    if (!orders.init()) {
        MessageBoxW(nullptr, L"Не удалось открыть базу данных bot.db",
                    L"Neira Bot Panel", MB_OK | MB_ICONERROR);
        return 1;
    }

    BotEngine engine(&config, &orders);

    // Create and show main window
    MainWindow mainWnd(hInst, &config, &engine, &orders);
    if (!mainWnd.create()) {
        MessageBoxW(nullptr, L"Не удалось создать главное окно.",
                    L"Neira Bot Panel", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(mainWnd.hwnd(), nCmdShow);
    UpdateWindow(mainWnd.hwnd());

    // Message loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
