#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QMap>
#include "configservice.h"

class MessagesPage : public QWidget
{
    Q_OBJECT
public:
    explicit MessagesPage(ConfigService *config, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void saveMessages();

private:
    void loadValues();

    ConfigService  *m_config;

    QPlainTextEdit *m_welcomeEdit;
    QPlainTextEdit *m_plansEdit;
    QPlainTextEdit *m_setupEdit;
    QPlainTextEdit *m_payInstrEdit;
    QPlainTextEdit *m_adminNotifyEdit;
    QPlainTextEdit *m_checkReceivedEdit;
    QPlainTextEdit *m_confirmedEdit;
    QPlainTextEdit *m_rejectedEdit;
};
