#include "mainwindow.h"

#include "pages/dashboardpage.h"
#include "pages/settingspage.h"
#include "pages/planspage.h"
#include "pages/paymentspage.h"
#include "pages/messagespage.h"
#include "pages/channelpage.h"
#include "pages/orderspage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Neira Bot Panel");
    resize(1100, 700);
    setupServices();
    setupUi();
}

MainWindow::~MainWindow()
{
    if (m_engine->isRunning())
        m_engine->stop();
    m_config->save();
}

void MainWindow::setupServices()
{
    m_config = new ConfigService(this);
    m_config->load();

    m_orders = new OrderService(this);
    m_orders->init();

    m_engine = new BotEngine(m_config, m_orders, this);
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- Sidebar ----
    m_sidebar = new QWidget(central);
    m_sidebar->setFixedWidth(200);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setStyleSheet("QWidget#sidebar { background-color: #161927; }");

    QVBoxLayout *sideLayout = new QVBoxLayout(m_sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    // Logo label
    QLabel *logo = new QLabel("🤖 Neira Panel", m_sidebar);
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedHeight(60);
    logo->setStyleSheet("font-size: 15px; font-weight: bold; color: #6ab0ff; "
                        "background: #111320; border-bottom: 1px solid #2a3550;");
    sideLayout->addWidget(logo);

    struct NavEntry { QString label; };
    QList<NavEntry> entries = {
        {"📊  Dashboard"},
        {"⚙️  Settings"},
        {"📋  Plans"},
        {"💳  Payments"},
        {"💬  Messages"},
        {"📢  Channel"},
        {"📦  Orders"}
    };

    for (int i = 0; i < entries.size(); ++i) {
        QPushButton *btn = makeNavButton(entries[i].label, i);
        sideLayout->addWidget(btn);
        m_navButtons.append(btn);
    }
    sideLayout->addStretch();

    mainLayout->addWidget(m_sidebar);

    // ---- Separator ----
    QFrame *sep = new QFrame(central);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(1);
    sep->setStyleSheet("color: #2a3550;");
    mainLayout->addWidget(sep);

    // ---- Pages ----
    m_stack = new QStackedWidget(central);

    m_dashPage     = new DashboardPage(m_config, m_engine, this);
    m_settingsPage = new SettingsPage(m_config, this);
    m_plansPage    = new PlansPage(m_config, this);
    m_paymentsPage = new PaymentsPage(m_config, this);
    m_messagesPage = new MessagesPage(m_config, this);
    m_channelPage  = new ChannelPage(m_config, m_engine, this);
    m_ordersPage   = new OrdersPage(m_orders, this);

    m_stack->addWidget(m_dashPage);
    m_stack->addWidget(m_settingsPage);
    m_stack->addWidget(m_plansPage);
    m_stack->addWidget(m_paymentsPage);
    m_stack->addWidget(m_messagesPage);
    m_stack->addWidget(m_channelPage);
    m_stack->addWidget(m_ordersPage);

    mainLayout->addWidget(m_stack, 1);

    // Connect orderUpdated -> refresh orders page
    connect(m_engine, &BotEngine::orderUpdated,
            m_ordersPage, &OrdersPage::refresh);

    switchPage(0);
}

QPushButton *MainWindow::makeNavButton(const QString &label, int index)
{
    QPushButton *btn = new QPushButton(label, m_sidebar);
    btn->setObjectName("navButton");
    btn->setCheckable(true);
    btn->setFixedHeight(46);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, [this, index]() {
        switchPage(index);
    });
    return btn;
}

void MainWindow::switchPage(int index)
{
    for (int i = 0; i < m_navButtons.size(); ++i)
        m_navButtons[i]->setChecked(i == index);
    m_stack->setCurrentIndex(index);
    m_currentPage = index;
}
