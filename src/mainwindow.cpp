#include "mainwindow.h"

#include "pages/dashboardpage.h"
#include "pages/settingspage.h"
#include "pages/planspage.h"
#include "pages/paymentspage.h"
#include "pages/messagespage.h"
#include "pages/channelpage.h"
#include "pages/orderspage.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

static const wchar_t* kMainClassName = L"NeiraMainWindow";
static const int SIDEBAR_W = 200;

// ---- MainWindow implementation ----

MainWindow::MainWindow(HINSTANCE hInst, ConfigService* config,
                       BotEngine* engine, OrderService* orders)
    : m_hInst(hInst), m_config(config), m_engine(engine), m_orders(orders)
{
    m_fontNormal  = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_fontBold    = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_sidebarBrush = CreateSolidBrush(RGB(30, 35, 55));
}

MainWindow::~MainWindow()
{
    if (m_fontNormal)   DeleteObject(m_fontNormal);
    if (m_fontBold)     DeleteObject(m_fontBold);
    if (m_sidebarBrush) DeleteObject(m_sidebarBrush);
}

bool MainWindow::create()
{
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = m_hInst;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kMainClassName;
    if (!RegisterClassExW(&wc)) return false;

    m_hwnd = CreateWindowExW(0, kMainClassName, L"Neira Bot Panel",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
        nullptr, nullptr, m_hInst, this);
    return m_hwnd != nullptr;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    MainWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_CREATE:
        self->onCreate();
        return 0;

    case WM_SIZE:
        self->onSize(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= ID_NAV_DASHBOARD && id <= ID_NAV_ORDERS) {
            self->switchPage(id - ID_NAV_DASHBOARD);
        }
        // Forward to current page
        if (self->m_pages[self->m_currentPage])
            SendMessageW(self->m_pages[self->m_currentPage], msg, wp, lp);
        return 0;
    }

    case WM_USER_LOG: {
        auto* text = reinterpret_cast<std::string*>(lp);
        if (text) {
            DashboardPage::appendLog(self->m_pages[PAGE_DASHBOARD], *text);
            delete text;
        }
        return 0;
    }

    case WM_USER_ORDER_UPDATED:
        if (self->m_pages[PAGE_ORDERS])
            OrdersPage::refresh(self->m_pages[PAGE_ORDERS], self->m_orders);
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HWND ctrl = reinterpret_cast<HWND>(lp);
        // Colour sidebar nav buttons
        for (int i = 0; i < PAGE_COUNT; ++i) {
            if (ctrl == self->m_navButtons[i]) {
                HDC hdc = reinterpret_cast<HDC>(wp);
                SetTextColor(hdc, RGB(200, 210, 230));
                SetBkColor(hdc, RGB(30, 35, 55));
                return reinterpret_cast<LRESULT>(self->m_sidebarBrush);
            }
        }
        break;
    }

    case WM_DESTROY:
        if (self->m_engine->isRunning())
            self->m_engine->stop();
        self->m_config->save();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MainWindow::onCreate()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int w = rc.right, h = rc.bottom;

    createSidebar(m_hwnd, SIDEBAR_W, h);

    // Content area
    RECT pageRc = { SIDEBAR_W + 1, 0, w, h };

    m_pages[PAGE_DASHBOARD] = DashboardPage::create(m_hwnd, pageRc, m_config, m_engine);
    m_pages[PAGE_SETTINGS]  = SettingsPage::create(m_hwnd, pageRc, m_config);
    m_pages[PAGE_PLANS]     = PlansPage::create(m_hwnd, pageRc, m_config);
    m_pages[PAGE_PAYMENTS]  = PaymentsPage::create(m_hwnd, pageRc, m_config);
    m_pages[PAGE_MESSAGES]  = MessagesPage::create(m_hwnd, pageRc, m_config);
    m_pages[PAGE_CHANNEL]   = ChannelPage::create(m_hwnd, pageRc, m_config, m_engine);
    m_pages[PAGE_ORDERS]    = OrdersPage::create(m_hwnd, pageRc, m_orders);

    // Wire orderUpdated callback
    m_engine->onOrderUpdated = [this]() {
        PostMessageW(m_hwnd, WM_USER_ORDER_UPDATED, 0, 0);
    };

    switchPage(PAGE_DASHBOARD);
}

void MainWindow::onSize(int w, int h)
{
    // Resize sidebar
    if (HWND sidebar = FindWindowExW(m_hwnd, nullptr, L"STATIC", nullptr)) {
        // We'll just reposition all nav buttons
    }

    int y = 60;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (m_navButtons[i])
            SetWindowPos(m_navButtons[i], nullptr, 0, y + i * 46, SIDEBAR_W, 46,
                         SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Resize page area
    RECT pageRc = { SIDEBAR_W + 1, 0, w, h };
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (m_pages[i])
            SetWindowPos(m_pages[i], nullptr,
                         pageRc.left, pageRc.top,
                         pageRc.right - pageRc.left, pageRc.bottom - pageRc.top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void MainWindow::createSidebar(HWND parent, int sidebarW, int h)
{
    // Logo label
    HWND logo = CreateWindowExW(0, L"STATIC", L"🤖 Neira Panel",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, sidebarW, 55,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(logo, WM_SETFONT, reinterpret_cast<WPARAM>(m_fontBold), TRUE);

    // Separator line
    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        0, 55, sidebarW, 2,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);

    struct NavEntry { UINT id; const char* label; };
    NavEntry entries[PAGE_COUNT] = {
        { ID_NAV_DASHBOARD, "📊  Dashboard"  },
        { ID_NAV_SETTINGS,  "⚙️   Settings"  },
        { ID_NAV_PLANS,     "📋  Plans"      },
        { ID_NAV_PAYMENTS,  "💳  Payments"   },
        { ID_NAV_MESSAGES,  "💬  Messages"   },
        { ID_NAV_CHANNEL,   "📢  Channel"    },
        { ID_NAV_ORDERS,    "��  Orders"     },
    };

    int y = 60;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        m_navButtons[i] = createButton(parent, entries[i].label, entries[i].id,
                                        0, y, sidebarW, 46);
        SendMessageW(m_navButtons[i], WM_SETFONT,
                     reinterpret_cast<WPARAM>(m_fontNormal), TRUE);
        y += 46;
    }
}

void MainWindow::switchPage(int index)
{
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (m_pages[i])
            ShowWindow(m_pages[i], i == index ? SW_SHOW : SW_HIDE);
    }
    m_currentPage = index;
    setSidebarButtonActive(index);

    // Trigger onShow equivalent
    if (m_pages[index])
        SendMessageW(m_pages[index], WM_SHOWWINDOW, TRUE, SW_PARENTOPENING);
}

void MainWindow::setSidebarButtonActive(int index)
{
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (m_navButtons[i])
            EnableWindow(m_navButtons[i], i != index);
    }
}
