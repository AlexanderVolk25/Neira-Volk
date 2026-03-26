#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include "configservice.h"
#include "botengine.h"
#include "orderservice.h"

class DashboardPage;
class SettingsPage;
class PlansPage;
class PaymentsPage;
class MessagesPage;
class ChannelPage;
class OrdersPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void setupServices();
    QPushButton *makeNavButton(const QString &label, int index);
    void switchPage(int index);

    ConfigService  *m_config;
    OrderService   *m_orders;
    BotEngine      *m_engine;

    QWidget        *m_sidebar;
    QStackedWidget *m_stack;

    DashboardPage  *m_dashPage;
    SettingsPage   *m_settingsPage;
    PlansPage      *m_plansPage;
    PaymentsPage   *m_paymentsPage;
    MessagesPage   *m_messagesPage;
    ChannelPage    *m_channelPage;
    OrdersPage     *m_ordersPage;

    QList<QPushButton*> m_navButtons;
    int m_currentPage = 0;
};
