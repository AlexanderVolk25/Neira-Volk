#pragma once

#include "utils.h"
#include "configservice.h"
#include "botengine.h"
#include "orderservice.h"

// Sidebar button IDs
#define ID_NAV_DASHBOARD  2001
#define ID_NAV_SETTINGS   2002
#define ID_NAV_PLANS      2003
#define ID_NAV_PAYMENTS   2004
#define ID_NAV_MESSAGES   2005
#define ID_NAV_CHANNEL    2006
#define ID_NAV_ORDERS     2007

// Page indices (must match button IDs - 2001)
enum PageIndex {
    PAGE_DASHBOARD = 0,
    PAGE_SETTINGS,
    PAGE_PLANS,
    PAGE_PAYMENTS,
    PAGE_MESSAGES,
    PAGE_CHANNEL,
    PAGE_ORDERS,
    PAGE_COUNT
};

// Forward declare page window registration functions
bool registerPageClasses(HINSTANCE hInst);

class MainWindow
{
public:
    MainWindow(HINSTANCE hInst, ConfigService* config,
               BotEngine* engine, OrderService* orders);
    ~MainWindow();

    bool create();
    HWND hwnd() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    void onCreate();
    void onSize(int w, int h);
    void switchPage(int index);
    void createSidebar(HWND parent, int sidebarW, int h);
    void setSidebarButtonActive(int index);

    HINSTANCE     m_hInst;
    HWND          m_hwnd = nullptr;

    ConfigService* m_config;
    BotEngine*     m_engine;
    OrderService*  m_orders;

    HWND m_navButtons[PAGE_COUNT] = {};
    HWND m_pages[PAGE_COUNT]      = {};
    int  m_currentPage            = PAGE_DASHBOARD;

    // Fonts & brushes for sidebar theming
    HFONT  m_fontNormal  = nullptr;
    HFONT  m_fontBold    = nullptr;
    HBRUSH m_sidebarBrush = nullptr;
};
