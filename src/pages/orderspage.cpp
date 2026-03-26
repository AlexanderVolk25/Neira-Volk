#include "orderspage.h"
#include <commctrl.h>

struct OrdersCtx { OrderService* orders; };

static void initOrdersListView(HWND lv)
{
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    wchar_t c0[] = L"#";         col.cx = 40;  col.pszText = c0; ListView_InsertColumn(lv, 0, &col);
    wchar_t c1[] = L"Пользователь"; col.cx = 130; col.pszText = c1; ListView_InsertColumn(lv, 1, &col);
    wchar_t c2[] = L"Тариф";     col.cx = 120; col.pszText = c2; ListView_InsertColumn(lv, 2, &col);
    wchar_t c3[] = L"Цена";      col.cx = 70;  col.pszText = c3; ListView_InsertColumn(lv, 3, &col);
    wchar_t c4[] = L"Статус";    col.cx = 100; col.pszText = c4; ListView_InsertColumn(lv, 4, &col);
    wchar_t c5[] = L"Создан";    col.cx = 160; col.pszText = c5; ListView_InsertColumn(lv, 5, &col);
}

static void fillOrders(HWND lv, const std::vector<Order>& orders)
{
    ListView_DeleteAllItems(lv);
    for (int i = 0; i < (int)orders.size(); ++i) {
        const Order& o = orders[i];
        LVITEMW item = {}; item.mask = LVIF_TEXT; item.iItem = i;
        std::wstring id = std::to_wstring(o.id);
        item.pszText = const_cast<wchar_t*>(id.c_str());
        ListView_InsertItem(lv, &item);

        auto setCol = [&](int c, const std::wstring& t) {
            wchar_t buf[256]; wcsncpy_s(buf, t.c_str(), _TRUNCATE);
            ListView_SetItemText(lv, i, c, buf);
        };
        std::string ustr = o.username.empty() ? std::to_string(o.userId) : "@" + o.username;
        setCol(1, toWide(ustr));
        setCol(2, toWide(o.planName));
        setCol(3, std::to_wstring(o.planPrice));
        setCol(4, toWide(o.status));
        setCol(5, toWide(o.createdAt));
    }
}

static LRESULT CALLBACK OrdersWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    OrdersCtx* ctx = reinterpret_cast<OrdersCtx*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        createLabel(hwnd, "Фильтр:", 20, 18, 60, 20);
        HWND combo = createComboBox(hwnd, IDC_ORDERS_FILTER, 84, 14, 180, 200);
        setDefaultFont(combo);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Все"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"pending"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"confirmed"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"rejected"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"delivered"));
        SendMessageW(combo, CB_SETCURSEL, 0, 0);

        HWND btnRef = createButton(hwnd, "🔄  Обновить", IDC_ORDERS_REFRESH, 280, 12, 130, 28);
        setDefaultFont(btnRef);

        HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER,
            20, 50, 820, 520,
            hwnd, reinterpret_cast<HMENU>((UINT_PTR)IDC_ORDERS_LIST),
            GetModuleHandleW(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        setDefaultFont(lv);
        initOrdersListView(lv);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        HWND lv = GetDlgItem(hwnd, IDC_ORDERS_LIST);
        if (lv) SetWindowPos(lv, nullptr, 20, 50, w-40, h-70, SWP_NOZORDER|SWP_NOACTIVATE);
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wp && ctx)
            OrdersPage::refresh(hwnd, ctx->orders);
        return 0;
    case WM_COMMAND:
        if (!ctx) break;
        if (LOWORD(wp) == IDC_ORDERS_REFRESH ||
            (LOWORD(wp) == IDC_ORDERS_FILTER && HIWORD(wp) == CBN_SELCHANGE))
        {
            OrdersPage::refresh(hwnd, ctx->orders);
        }
        return 0;
    case WM_DESTROY:
        delete ctx;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

namespace OrdersPage {

bool registerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = OrdersWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NeiraPageOrders";
    return RegisterClassExW(&wc) != 0;
}

HWND create(HWND parent, const RECT& rc, OrderService* orders)
{
    HWND hwnd = CreateWindowExW(0, L"NeiraPageOrders", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    auto* ctx = new OrdersCtx{ orders };
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    return hwnd;
}

void refresh(HWND hwnd, OrderService* orders)
{
    HWND combo = GetDlgItem(hwnd, IDC_ORDERS_FILTER);
    HWND lv    = GetDlgItem(hwnd, IDC_ORDERS_LIST);
    if (!combo || !lv) return;

    int sel = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    std::vector<Order> list;
    switch (sel) {
    case 0:  list = orders->getAllOrders();                    break;
    case 1:  list = orders->getOrdersByStatus("pending");     break;
    case 2:  list = orders->getOrdersByStatus("confirmed");   break;
    case 3:  list = orders->getOrdersByStatus("rejected");    break;
    case 4:  list = orders->getOrdersByStatus("delivered");   break;
    default: list = orders->getAllOrders();                    break;
    }
    fillOrders(lv, list);
}

} // namespace OrdersPage
