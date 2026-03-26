#pragma once

#include <QWidget>
#include <QTableWidget>
#include "configservice.h"

class PlansPage : public QWidget
{
    Q_OBJECT
public:
    explicit PlansPage(ConfigService *config, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void addRow();
    void removeRow();
    void savePlans();

private:
    void loadPlans();
    void setRowWidgets(int row, int typeIndex, int value);

    ConfigService  *m_config;
    QTableWidget   *m_table;
};
