#include "planspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QShowEvent>
#include <QMessageBox>

static const QStringList DURATION_TYPES = {"дней", "месяцев", "лет"};

PlansPage::PlansPage(ConfigService *config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel *title = new QLabel("📋 Plans", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Название", "Цена (₽)", "Тип длительности", "Значение"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 100);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 160);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 100);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(m_table, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *addBtn    = new QPushButton("➕ Добавить", this);
    QPushButton *removeBtn = new QPushButton("🗑 Удалить", this);
    QPushButton *saveBtn   = new QPushButton("💾 Сохранить", this);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    root->addLayout(btnLayout);

    connect(addBtn,    &QPushButton::clicked, this, &PlansPage::addRow);
    connect(removeBtn, &QPushButton::clicked, this, &PlansPage::removeRow);
    connect(saveBtn,   &QPushButton::clicked, this, &PlansPage::savePlans);

    loadPlans();
}

void PlansPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadPlans();
}

void PlansPage::loadPlans()
{
    m_table->setRowCount(0);
    const QList<PlanConfig> &plans = m_config->plans();
    for (const PlanConfig &p : plans) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(p.name));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(p.price)));

        QComboBox *typeCombo = new QComboBox(m_table);
        typeCombo->addItems(DURATION_TYPES);
        // Determine type
        if (p.years > 0) {
            typeCombo->setCurrentIndex(2);
            m_table->setCellWidget(row, 2, typeCombo);
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(p.years)));
        } else if (p.months > 0) {
            typeCombo->setCurrentIndex(1);
            m_table->setCellWidget(row, 2, typeCombo);
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(p.months)));
        } else {
            typeCombo->setCurrentIndex(0);
            m_table->setCellWidget(row, 2, typeCombo);
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(p.days)));
        }
    }
}

void PlansPage::addRow()
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem("Новый тариф"));
    m_table->setItem(row, 1, new QTableWidgetItem("100"));
    QComboBox *typeCombo = new QComboBox(m_table);
    typeCombo->addItems(DURATION_TYPES);
    m_table->setCellWidget(row, 2, typeCombo);
    m_table->setItem(row, 3, new QTableWidgetItem("1"));
}

void PlansPage::removeRow()
{
    int row = m_table->currentRow();
    if (row >= 0)
        m_table->removeRow(row);
}

void PlansPage::savePlans()
{
    QList<PlanConfig> plans;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        PlanConfig p;
        p.name  = m_table->item(row, 0) ? m_table->item(row, 0)->text().trimmed() : "";
        p.price = m_table->item(row, 1) ? m_table->item(row, 1)->text().toInt() : 0;

        QComboBox *typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(row, 2));
        int typeIdx = typeCombo ? typeCombo->currentIndex() : 0;
        int value   = m_table->item(row, 3) ? m_table->item(row, 3)->text().toInt() : 1;

        if (typeIdx == 2)      p.years  = value;
        else if (typeIdx == 1) p.months = value;
        else                   p.days   = value;

        if (!p.name.isEmpty())
            plans.append(p);
    }
    m_config->setPlans(plans);
    m_config->save();
    QMessageBox::information(this, "Сохранено", "Тарифы сохранены.");
}
