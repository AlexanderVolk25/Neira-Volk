#include "paymentspage.h"
#include <commctrl.h>

struct PayCtx { ConfigService* config; };

static void initProvTable(HWND lv)
{
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    wchar_t c0[] = L"Провайдер"; col.cx = 130; col.pszText = c0; ListView_InsertColumn(lv, 0, &col);
    wchar_t c1[] = L"Включён";   col.cx = 80;  col.pszText = c1; ListView_InsertColumn(lv, 1, &col);
    wchar_t c2[] = L"API Key";   col.cx = 180; col.pszText = c2; ListView_InsertColumn(lv, 2, &col);
    wchar_t c3[] = L"Terminal";  col.cx = 130; col.pszText = c3; ListView_InsertColumn(lv, 3, &col);
}

static LRESULT CALLBACK PayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PayCtx* ctx = reinterpret_cast<PayCtx*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        createLabel(hwnd, "Ссылка для ручной оплаты:", 20, 15, 250, 22);
        HWND linkEd = createEdit(hwnd, IDC_PAY_LINK_EDIT, 20, 38, 500, 24);
        setDefaultFont(linkEd);
        HWND btnSaveLink = createButton(hwnd, "💾  Сохранить ссылку", IDC_PAY_LINK_SAVE, 20, 72, 190, 30);
        setDefaultFont(btnSaveLink);

        createLabel(hwnd, "Провайдеры автооплаты:", 20, 120, 240, 22);
        HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
            20, 144, 560, 240,
            hwnd, reinterpret_cast<HMENU>((UINT_PTR)IDC_PAY_PROV_LIST),
            GetModuleHandleW(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        setDefaultFont(lv);
        initProvTable(lv);

        HWND btnSaveProv = createButton(hwnd, "💾  Сохранить провайдеры", IDC_PAY_PROV_SAVE, 20, 400, 220, 30);
        setDefaultFont(btnSaveProv);
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wp && ctx)
            PaymentsPage::loadValues(hwnd, ctx->config);
        return 0;
    case WM_COMMAND:
        if (!ctx) break;
        if (LOWORD(wp) == IDC_PAY_LINK_SAVE) {
            ctx->config->setManualPayLink(getWindowText(GetDlgItem(hwnd, IDC_PAY_LINK_EDIT)));
            ctx->config->save();
            MessageBoxW(hwnd, L"Ссылка сохранена!", L"Neira Bot Panel", MB_OK | MB_ICONINFORMATION);
        } else if (LOWORD(wp) == IDC_PAY_PROV_SAVE) {
            HWND lv = GetDlgItem(hwnd, IDC_PAY_PROV_LIST);
            int count = ListView_GetItemCount(lv);
            auto providers = ctx->config->autoProviders();
            wchar_t buf[256];
            for (int i = 0; i < count && i < (int)providers.size(); ++i) {
                ListView_GetItemText(lv, i, 1, buf, 256);
                providers[i].enabled = (wcsncmp(buf, L"Да", 2) == 0);
                ListView_GetItemText(lv, i, 2, buf, 256);
                providers[i].apiKey = toUtf8(buf);
                ListView_GetItemText(lv, i, 3, buf, 256);
                providers[i].terminalKey = toUtf8(buf);
            }
            ctx->config->setAutoProviders(providers);
            ctx->config->save();
            MessageBoxW(hwnd, L"Провайдеры сохранены!", L"Neira Bot Panel", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace PaymentsPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = PayWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPagePayments";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc, ConfigService* config)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPagePayments", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    auto* ctx = new PayCtx{ config };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    return hwnd;
}

void loadValues(HWND hwnd, ConfigService* config)
{
    setWindowText(GetDlgItem(hwnd, IDC_PAY_LINK_EDIT), config->manualPayLink());
    HWND lv = GetDlgItem(hwnd, IDC_PAY_PROV_LIST);
    if (!lv) return;
    ListView_DeleteAllItems(lv);
    const auto& provs = config->autoProviders();
    for (int i = 0; i < (int)provs.size(); ++i) {
        const auto& p = provs[i];
        LVITEMW item = {}; item.mask = LVIF_TEXT; item.iItem = i;
        std::wstring name = toWide(p.displayName);
        item.pszText = const_cast<wchar_t*>(name.c_str());
        ListView_InsertItem(lv, &item);

        auto setCol = [&](int c, const std::wstring& t) {
            wchar_t buf[256]; wcsncpy_s(buf, t.c_str(), _TRUNCATE);
            ListView_SetItemText(lv, i, c, buf);
        };
        setCol(1, p.enabled ? L"Да" : L"Нет");
        setCol(2, toWide(p.apiKey));
        setCol(3, toWide(p.terminalKey));
    }
}

} // namespace PaymentsPage
