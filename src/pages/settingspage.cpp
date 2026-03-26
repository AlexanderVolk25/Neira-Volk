#include "settingspage.h"

struct SettingsCtx { ConfigService* config; };

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    SettingsCtx* ctx = reinterpret_cast<SettingsCtx*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        int y = 20, dy = 50;
        auto row = [&](const std::string& label, UINT id) {
            HWND lbl = createLabel(hwnd, label, 20, y, 220, 20);
            setDefaultFont(lbl);
            HWND ed  = createEdit(hwnd, id, 20, y + 22, 500, 24);
            setDefaultFont(ed);
            y += dy;
        };
        row("Токен бота:", IDC_SET_TOKEN_EDIT);
        row("ID администратора:", IDC_SET_ADMIN_EDIT);
        row("Username поддержки (без @):", IDC_SET_SUPPORT_EDIT);
        row("Username канала (без @):", IDC_SET_CHANNEL_EDIT);

        HWND chk = createCheckbox(hwnd, "Показывать онлайн-оплату", IDC_SET_AUTOPAY_CHK,
                                   20, y, 260, 22);
        setDefaultFont(chk);
        y += 40;

        HWND btn = createButton(hwnd, "💾  Сохранить", IDC_SET_SAVE, 20, y, 140, 32);
        setDefaultFont(btn);
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wp && ctx)
            SettingsPage::loadValues(hwnd, ctx->config);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_SET_SAVE && ctx) {
            ctx->config->setBotToken(getWindowText(GetDlgItem(hwnd, IDC_SET_TOKEN_EDIT)));
            try {
                ctx->config->setAdminId(std::stoll(
                    getWindowText(GetDlgItem(hwnd, IDC_SET_ADMIN_EDIT))));
            } catch (...) {}
            ctx->config->setSupportUsername(getWindowText(GetDlgItem(hwnd, IDC_SET_SUPPORT_EDIT)));
            ctx->config->setChannelUsername(getWindowText(GetDlgItem(hwnd, IDC_SET_CHANNEL_EDIT)));
            ctx->config->setShowAutoPayment(
                SendDlgItemMessageW(hwnd, IDC_SET_AUTOPAY_CHK, BM_GETCHECK, 0, 0) == BST_CHECKED);
            ctx->config->save();
            MessageBoxW(hwnd, L"Настройки сохранены!", L"Neira Bot Panel", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace SettingsPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = SettingsWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPageSettings";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc, ConfigService* config)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPageSettings", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);

    auto* ctx = new SettingsCtx{ config };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    return hwnd;
}

void loadValues(HWND hwnd, ConfigService* config)
{
    setWindowText(GetDlgItem(hwnd, IDC_SET_TOKEN_EDIT),   config->botToken());
    setWindowText(GetDlgItem(hwnd, IDC_SET_ADMIN_EDIT),
        config->adminId() ? std::to_string(config->adminId()) : "");
    setWindowText(GetDlgItem(hwnd, IDC_SET_SUPPORT_EDIT), config->supportUsername());
    setWindowText(GetDlgItem(hwnd, IDC_SET_CHANNEL_EDIT), config->channelUsername());
    CheckDlgButton(hwnd, IDC_SET_AUTOPAY_CHK,
        config->showAutoPayment() ? BST_CHECKED : BST_UNCHECKED);
}

} // namespace SettingsPage
