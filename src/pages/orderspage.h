#pragma once
#include "../utils.h"
#include "../orderservice.h"

#define IDC_ORDERS_FILTER  3601
#define IDC_ORDERS_LIST    3602
#define IDC_ORDERS_REFRESH 3603

namespace OrdersPage {
    bool registerClass(HINSTANCE hInst);
    HWND create(HWND parent, const RECT& rc, OrderService* orders);
    void refresh(HWND hwnd, OrderService* orders);
}
