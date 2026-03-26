#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include "configservice.h"

class PaymentsPage : public QWidget
{
    Q_OBJECT
public:
    explicit PaymentsPage(ConfigService *config, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void saveManualLink();
    void saveProviders();

private:
    void loadValues();

    ConfigService  *m_config;
    QLineEdit      *m_linkEdit;
    QTableWidget   *m_provTable;
};
