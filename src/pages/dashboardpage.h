#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>

#include "configservice.h"
#include "botengine.h"

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(ConfigService *config, BotEngine *engine,
                           QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &text);

private slots:
    void onStartStop();
    void clearLogs();

private:
    void updateStatus(bool running);

    ConfigService   *m_config;
    BotEngine       *m_engine;

    QLabel          *m_statusDot;
    QLabel          *m_statusLabel;
    QPushButton     *m_startStopBtn;
    QPlainTextEdit  *m_logView;
};
