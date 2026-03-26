#include "dashboardpage.h"
#include <string>
#include <thread>

struct DashCtx {
    ConfigService* config;
    BotEngine*     engine;
};

static LRESULT CALLBACK DashWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    DashCtx* ctx = reinterpret_cast<DashCtx*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        // Status row
        createLabel(hwnd, "Статус бота:", 20, 20, 100, 22);
        HWND dot   = createLabel(hwnd, "●", 125, 20, 20, 22, WS_CHILD|WS_VISIBLE|SS_LEFT);
        HWND label = createLabel(hwnd, "Остановлен", 150, 20, 180, 22);
        SetWindowLongPtrW(dot,   GWLP_ID, IDC_DASH_STATUS_DOT);
        SetWindowLongPtrW(label, GWLP_ID, IDC_DASH_STATUS_LABEL);
        setDefaultFont(dot);
        setDefaultFont(label);

        // Buttons
        HWND btnSS = createButton(hwnd, "▶  Запустить", IDC_DASH_STARTSTOP,
                                  20, 55, 130, 32);
        HWND btnClear = createButton(hwnd, "🗑  Очистить лог", IDC_DASH_CLEAR,
                                     165, 55, 150, 32);
        setDefaultFont(btnSS);
        setDefaultFont(btnClear);

        // Log area
        createLabel(hwnd, "Лог работы бота:", 20, 100, 200, 22);
        HWND log = createEdit(hwnd, IDC_DASH_LOG, 20, 125, 820, 480, true, true);
        setDefaultFont(log);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        HWND log = GetDlgItem(hwnd, IDC_DASH_LOG);
        if (log) SetWindowPos(log, nullptr, 20, 125, w - 40, h - 145,
                               SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_COMMAND:
        if (!ctx) break;
        if (LOWORD(wp) == IDC_DASH_STARTSTOP) {
            if (ctx->engine->isRunning()) {
                ctx->engine->stop();
                SetDlgItemTextW(hwnd, IDC_DASH_STARTSTOP, L"▶  Запустить");
                SetDlgItemTextW(hwnd, IDC_DASH_STATUS_LABEL, L"Остановлен");
                SetDlgItemTextW(hwnd, IDC_DASH_STATUS_DOT,   L"●");
                // Red color via custom draw – we just show text
            } else {
                ctx->engine->start();
                SetDlgItemTextW(hwnd, IDC_DASH_STARTSTOP, L"⏹  Остановить");
                SetDlgItemTextW(hwnd, IDC_DASH_STATUS_LABEL, L"Работает");
                SetDlgItemTextW(hwnd, IDC_DASH_STATUS_DOT,   L"●");
            }
        } else if (LOWORD(wp) == IDC_DASH_CLEAR) {
            SetDlgItemTextW(hwnd, IDC_DASH_LOG, L"");
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace DashboardPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = DashWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPageDashboard";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc,
            ConfigService* config, BotEngine* engine)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPageDashboard", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);

    auto* ctx = new DashCtx{ config, engine };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    // Connect bot log callback -> post to parent (main window)
    engine->onLog = [parent](const std::string& text) {
        auto* msg = new std::string(text);
        PostMessageW(parent, WM_USER_LOG, 0, reinterpret_cast<LPARAM>(msg));
    };

    return hwnd;
}

void appendLog(HWND hwnd, const std::string& text)
{
    HWND log = GetDlgItem(hwnd, IDC_DASH_LOG);
    if (!log) return;
    int len = GetWindowTextLengthW(log);
    SendMessageW(log, EM_SETSEL, len, len);
    std::wstring wtext = toWide(text) + L"\r\n";
    SendMessageW(log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(wtext.c_str()));
    SendMessageW(log, EM_SCROLL, SB_BOTTOM, 0);
}

} // namespace DashboardPage
