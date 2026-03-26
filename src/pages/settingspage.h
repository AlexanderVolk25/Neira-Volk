#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>

#include "configservice.h"

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(ConfigService *config, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void saveSettings();

private:
    void loadValues();

    ConfigService *m_config;

    QLineEdit *m_tokenEdit;
    QLineEdit *m_adminIdEdit;
    QLineEdit *m_supportEdit;
    QLineEdit *m_channelEdit;
    QCheckBox *m_autoPayCheck;
};
