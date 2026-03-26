#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include "configservice.h"
#include "botengine.h"

class ChannelPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChannelPage(ConfigService *config, BotEngine *engine,
                         QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void publishPost();

private:
    ConfigService  *m_config;
    BotEngine      *m_engine;

    QPlainTextEdit *m_postEdit;
    QLabel         *m_statusLabel;
};
