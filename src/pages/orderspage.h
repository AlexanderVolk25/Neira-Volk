#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include "orderservice.h"

class OrdersPage : public QWidget
{
    Q_OBJECT
public:
    explicit OrdersPage(OrderService *orders, QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    void loadOrders(const QString &statusFilter = "all");

    OrderService   *m_orders;
    QTableWidget   *m_table;
    QComboBox      *m_filterCombo;
};
