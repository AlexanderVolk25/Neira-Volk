#include "dashboardpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>

DashboardPage::DashboardPage(ConfigService *config, BotEngine *engine, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
    , m_engine(engine)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Title
    QLabel *title = new QLabel("📊 Dashboard", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    // Status group
    QGroupBox *statusGroup = new QGroupBox("Статус бота", this);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(12);

    m_statusDot = new QLabel("●", statusGroup);
    m_statusDot->setFixedSize(20, 20);

    m_statusLabel = new QLabel("Offline", statusGroup);
    m_statusLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    m_startStopBtn = new QPushButton("▶ Запустить", statusGroup);
    m_startStopBtn->setFixedWidth(140);

    statusLayout->addWidget(m_statusDot);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_startStopBtn);

    root->addWidget(statusGroup);

    // Log group
    QGroupBox *logGroup = new QGroupBox("Логи", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    m_logView = new QPlainTextEdit(logGroup);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(1000);
    m_logView->setPlaceholderText("Логи появятся здесь после запуска бота...");
    logLayout->addWidget(m_logView);

    QPushButton *clearBtn = new QPushButton("🗑 Очистить логи", logGroup);
    clearBtn->setFixedWidth(160);
    logLayout->addWidget(clearBtn, 0, Qt::AlignRight);

    root->addWidget(logGroup, 1);

    // Connections
    connect(m_startStopBtn, &QPushButton::clicked, this, &DashboardPage::onStartStop);
    connect(clearBtn, &QPushButton::clicked, this, &DashboardPage::clearLogs);
    connect(m_engine, &BotEngine::logMessage, this, &DashboardPage::appendLog);

    updateStatus(false);
}

void DashboardPage::onStartStop()
{
    if (m_engine->isRunning()) {
        m_engine->stop();
        updateStatus(false);
    } else {
        m_engine->start();
        updateStatus(true);
    }
}

void DashboardPage::appendLog(const QString &text)
{
    const QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    m_logView->appendPlainText(timestamp + text);
}

void DashboardPage::clearLogs()
{
    m_logView->clear();
}

void DashboardPage::updateStatus(bool running)
{
    if (running) {
        m_statusDot->setStyleSheet("color: #4caf50; font-size: 18px;");
        m_statusLabel->setText("Online");
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #4caf50;");
        m_startStopBtn->setText("⏹ Остановить");
        m_startStopBtn->setStyleSheet("background: #c0392b; color: white; border-radius: 5px; padding: 8px 16px;");
    } else {
        m_statusDot->setStyleSheet("color: #e74c3c; font-size: 18px;");
        m_statusLabel->setText("Offline");
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #e74c3c;");
        m_startStopBtn->setText("▶ Запустить");
        m_startStopBtn->setStyleSheet("");
    }
}
