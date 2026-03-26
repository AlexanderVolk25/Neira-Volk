#include "planspage.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

struct PlansCtx { ConfigService* config; };

// ListView columns: Name | Price | Days | Months | Years
static void initListView(HWND lv)
{
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    wchar_t c0[] = L"Название";      col.cx = 200; col.pszText = c0; ListView_InsertColumn(lv, 0, &col);
    wchar_t c1[] = L"Цена (₽)";     col.cx = 90;  col.pszText = c1; ListView_InsertColumn(lv, 1, &col);
    wchar_t c2[] = L"Дней";          col.cx = 70;  col.pszText = c2; ListView_InsertColumn(lv, 2, &col);
    wchar_t c3[] = L"Месяцев";       col.cx = 80;  col.pszText = c3; ListView_InsertColumn(lv, 3, &col);
    wchar_t c4[] = L"Лет";           col.cx = 60;  col.pszText = c4; ListView_InsertColumn(lv, 4, &col);
}

static void fillListView(HWND lv, ConfigService* config)
{
    ListView_DeleteAllItems(lv);
    const auto& plans = config->plans();
    for (int i = 0; i < (int)plans.size(); ++i) {
        const PlanConfig& p = plans[i];
        LVITEMW item = {};
        item.mask    = LVIF_TEXT;
        item.iItem   = i;

        std::wstring name = toWide(p.name);
        item.pszText = const_cast<wchar_t*>(name.c_str());
        ListView_InsertItem(lv, &item);

        auto setCol = [&](int col, const std::wstring& text) {
            wchar_t buf[64];
            wcsncpy_s(buf, text.c_str(), _TRUNCATE);
            ListView_SetItemText(lv, i, col, buf);
        };
        setCol(1, std::to_wstring(p.price));
        setCol(2, std::to_wstring(p.days));
        setCol(3, std::to_wstring(p.months));
        setCol(4, std::to_wstring(p.years));
    }
}

static LRESULT CALLBACK PlansWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PlansCtx* ctx = reinterpret_cast<PlansCtx*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        createLabel(hwnd, "Тарифные планы (редактируйте двойным кликом):", 20, 15, 500, 22);
        HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
            20, 40, 540, 360,
            hwnd, reinterpret_cast<HMENU>((UINT_PTR)IDC_PLANS_LIST),
            GetModuleHandleW(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        setDefaultFont(lv);
        initListView(lv);

        HWND btnAdd = createButton(hwnd, "➕  Добавить", IDC_PLANS_ADD,    20, 415, 130, 30);
        HWND btnDel = createButton(hwnd, "➖  Удалить",  IDC_PLANS_REMOVE, 165, 415, 130, 30);
        HWND btnSave = createButton(hwnd, "💾  Сохранить", IDC_PLANS_SAVE, 310, 415, 140, 30);
        setDefaultFont(btnAdd); setDefaultFont(btnDel); setDefaultFont(btnSave);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        HWND lv = GetDlgItem(hwnd, IDC_PLANS_LIST);
        if (lv) SetWindowPos(lv, nullptr, 20, 40, w-40, h-110, SWP_NOZORDER|SWP_NOACTIVATE);
        // Reposition buttons
        int btnY = h - 55;
        if (GetDlgItem(hwnd, IDC_PLANS_ADD))
            SetWindowPos(GetDlgItem(hwnd, IDC_PLANS_ADD),    nullptr, 20,  btnY, 130, 30, SWP_NOZORDER|SWP_NOACTIVATE);
        if (GetDlgItem(hwnd, IDC_PLANS_REMOVE))
            SetWindowPos(GetDlgItem(hwnd, IDC_PLANS_REMOVE), nullptr, 165, btnY, 130, 30, SWP_NOZORDER|SWP_NOACTIVATE);
        if (GetDlgItem(hwnd, IDC_PLANS_SAVE))
            SetWindowPos(GetDlgItem(hwnd, IDC_PLANS_SAVE),   nullptr, 310, btnY, 140, 30, SWP_NOZORDER|SWP_NOACTIVATE);
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wp && ctx)
            PlansPage::loadValues(hwnd, ctx->config);
        return 0;
    case WM_COMMAND:
        if (!ctx) break;
        if (LOWORD(wp) == IDC_PLANS_ADD) {
            auto plans = ctx->config->plans();
            plans.push_back({"Новый тариф", 0, 0, 1, 0});
            ctx->config->setPlans(plans);
            PlansPage::loadValues(hwnd, ctx->config);
        } else if (LOWORD(wp) == IDC_PLANS_REMOVE) {
            HWND lv = GetDlgItem(hwnd, IDC_PLANS_LIST);
            int sel = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
            if (sel >= 0) {
                auto plans = ctx->config->plans();
                if (sel < (int)plans.size()) {
                    plans.erase(plans.begin() + sel);
                    ctx->config->setPlans(plans);
                    PlansPage::loadValues(hwnd, ctx->config);
                }
            }
        } else if (LOWORD(wp) == IDC_PLANS_SAVE) {
            // Read back from list view
            HWND lv = GetDlgItem(hwnd, IDC_PLANS_LIST);
            int count = ListView_GetItemCount(lv);
            std::vector<PlanConfig> plans(count);
            wchar_t buf[256];
            for (int i = 0; i < count; ++i) {
                ListView_GetItemText(lv, i, 0, buf, 256); plans[i].name   = toUtf8(buf);
                ListView_GetItemText(lv, i, 1, buf, 256); try { plans[i].price  = std::stoi(toUtf8(buf)); } catch(...) {}
                ListView_GetItemText(lv, i, 2, buf, 256); try { plans[i].days   = std::stoi(toUtf8(buf)); } catch(...) {}
                ListView_GetItemText(lv, i, 3, buf, 256); try { plans[i].months = std::stoi(toUtf8(buf)); } catch(...) {}
                ListView_GetItemText(lv, i, 4, buf, 256); try { plans[i].years  = std::stoi(toUtf8(buf)); } catch(...) {}
            }
            ctx->config->setPlans(plans);
            ctx->config->save();
            MessageBoxW(hwnd, L"Тарифы сохранены!", L"Neira Bot Panel", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace PlansPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = PlansWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPagePlans";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc, ConfigService* config)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPagePlans", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);

    auto* ctx = new PlansCtx{ config };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    return hwnd;
}

void loadValues(HWND hwnd, ConfigService* config)
{
    HWND lv = GetDlgItem(hwnd, IDC_PLANS_LIST);
    if (lv) fillListView(lv, config);
}

} // namespace PlansPage
