#include "messagespage.h"

struct MsgCtx { ConfigService* config; };

static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    MsgCtx* ctx = reinterpret_cast<MsgCtx*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        // Two-column layout: 8 message editors
        struct EdEntry { const char* label; UINT id; };
        EdEntry entries[] = {
            { "Приветствие (/start):",            IDC_MSG_WELCOME_EDIT    },
            { "Список тарифов ({plans}):",         IDC_MSG_PLANS_EDIT      },
            { "Инструкция по настройке:",          IDC_MSG_SETUP_EDIT      },
            { "Инструкция по оплате:",             IDC_MSG_PAYINSTR_EDIT   },
            { "Уведомление администратору:",       IDC_MSG_ADMINNOTIFY_EDIT},
            { "Чек получен:",                      IDC_MSG_CHECKRECEIVED_EDIT },
            { "Заказ подтверждён ({proxy_data}):", IDC_MSG_CONFIRMED_EDIT  },
            { "Заказ отклонён:",                   IDC_MSG_REJECTED_EDIT   },
        };
        int col0 = 20, col1 = 430;
        int y0   = 15, dy = 155;
        for (int i = 0; i < 8; ++i) {
            int col = (i % 2 == 0) ? col0 : col1;
            int row = i / 2;
            int y   = y0 + row * dy;
            HWND lbl = createLabel(hwnd, entries[i].label, col, y, 380, 20);
            HWND ed  = createEdit(hwnd, entries[i].id, col, y+22, 380, 120, true);
            setDefaultFont(lbl);
            setDefaultFont(ed);
        }
        HWND btn = createButton(hwnd, "💾  Сохранить все сообщения", IDC_MSG_SAVE,
                                 20, 15 + 4*dy, 250, 30);
        setDefaultFont(btn);
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wp && ctx)
            MessagesPage::loadValues(hwnd, ctx->config);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_MSG_SAVE && ctx) {
            ctx->config->setWelcomeMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_WELCOME_EDIT)));
            ctx->config->setPlansMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_PLANS_EDIT)));
            ctx->config->setHowToSetupMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_SETUP_EDIT)));
            ctx->config->setPaymentInstructionMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_PAYINSTR_EDIT)));
            ctx->config->setAdminNotifyMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_ADMINNOTIFY_EDIT)));
            ctx->config->setCheckReceivedMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_CHECKRECEIVED_EDIT)));
            ctx->config->setOrderConfirmedMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_CONFIRMED_EDIT)));
            ctx->config->setOrderRejectedMsg(getWindowText(GetDlgItem(hwnd, IDC_MSG_REJECTED_EDIT)));
            ctx->config->save();
            MessageBoxW(hwnd, L"Сообщения сохранены!", L"Neira Bot Panel", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace MessagesPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPageMessages";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc, ConfigService* config)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPageMessages", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    auto* ctx = new MsgCtx{ config };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    return hwnd;
}

void loadValues(HWND hwnd, ConfigService* config)
{
    setWindowText(GetDlgItem(hwnd, IDC_MSG_WELCOME_EDIT),       config->welcomeMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_PLANS_EDIT),         config->plansMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_SETUP_EDIT),         config->howToSetupMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_PAYINSTR_EDIT),      config->paymentInstructionMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_ADMINNOTIFY_EDIT),   config->adminNotifyMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_CHECKRECEIVED_EDIT), config->checkReceivedMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_CONFIRMED_EDIT),     config->orderConfirmedMsg());
    setWindowText(GetDlgItem(hwnd, IDC_MSG_REJECTED_EDIT),      config->orderRejectedMsg());
}

} // namespace MessagesPage
