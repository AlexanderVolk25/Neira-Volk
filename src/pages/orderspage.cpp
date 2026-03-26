#include "orderspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QHeaderView>

OrdersPage::OrdersPage(OrderService *orders, QWidget *parent)
    : QWidget(parent)
    , m_orders(orders)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel *title = new QLabel("📦 Orders", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    // Filter bar
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);
    QLabel *filterLabel = new QLabel("Фильтр:", this);
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItems({"Все", "pending", "confirmed", "rejected", "delivered"});
    m_filterCombo->setFixedWidth(140);

    QPushButton *refreshBtn = new QPushButton("🔄 Обновить", this);
    refreshBtn->setFixedWidth(130);

    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(m_filterCombo);
    filterLayout->addWidget(refreshBtn);
    filterLayout->addStretch();
    root->addLayout(filterLayout);

    // Table
    m_table = new QTableWidget(0, 7, this);
    m_table->setHorizontalHeaderLabels({
        "ID", "User ID", "Username", "Plan", "Price (₽)", "Status", "Created"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table, 1);

    connect(refreshBtn, &QPushButton::clicked, this, &OrdersPage::refresh);
    connect(m_filterCombo, &QComboBox::currentTextChanged,
            this, [this](const QString &text) {
                loadOrders(text == "Все" ? "all" : text);
            });

    loadOrders("all");
}

void OrdersPage::refresh()
{
    const QString filter = m_filterCombo->currentText();
    loadOrders(filter == "Все" ? "all" : filter);
}

void OrdersPage::loadOrders(const QString &statusFilter)
{
    QList<Order> orders;
    if (statusFilter == "all")
        orders = m_orders->getAllOrders();
    else
        orders = m_orders->getOrdersByStatus(statusFilter);

    m_table->setRowCount(0);
    for (const Order &o : orders) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto mkItem = [](const QString &t) {
            QTableWidgetItem *it = new QTableWidgetItem(t);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            return it;
        };

        m_table->setItem(row, 0, mkItem(QString::number(o.id)));
        m_table->setItem(row, 1, mkItem(QString::number(o.userId)));
        m_table->setItem(row, 2, mkItem(o.username));
        m_table->setItem(row, 3, mkItem(o.planName));
        m_table->setItem(row, 4, mkItem(QString::number(o.planPrice)));

        QTableWidgetItem *statusItem = mkItem(o.status);
        if (o.status == "pending")    statusItem->setForeground(QColor("#f0a000"));
        else if (o.status == "confirmed" || o.status == "delivered")
                                       statusItem->setForeground(QColor("#4caf50"));
        else if (o.status == "rejected") statusItem->setForeground(QColor("#e74c3c"));
        m_table->setItem(row, 5, statusItem);

        m_table->setItem(row, 6, mkItem(o.createdAt));
    }
}
