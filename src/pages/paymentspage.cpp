#include "paymentspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QCheckBox>
#include <QHeaderView>
#include <QShowEvent>
#include <QMessageBox>

PaymentsPage::PaymentsPage(ConfigService *config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel *title = new QLabel("💳 Payments", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    // ---- Manual payment section ----
    QGroupBox *manualGroup = new QGroupBox("Ручная оплата", this);
    QVBoxLayout *manualLayout = new QVBoxLayout(manualGroup);

    QLabel *linkLabel = new QLabel("Ссылка на оплату (единая для всех тарифов):", manualGroup);
    m_linkEdit = new QLineEdit(manualGroup);
    m_linkEdit->setPlaceholderText("https://www.tinkoff.ru/rm/...");

    QPushButton *saveLinkBtn = new QPushButton("💾 Сохранить ссылку", manualGroup);
    saveLinkBtn->setFixedWidth(180);

    manualLayout->addWidget(linkLabel);
    manualLayout->addWidget(m_linkEdit);
    manualLayout->addWidget(saveLinkBtn, 0, Qt::AlignLeft);

    root->addWidget(manualGroup);

    // ---- Auto payment providers section ----
    QGroupBox *autoGroup = new QGroupBox("Авто-оплата (провайдеры)", this);
    QVBoxLayout *autoLayout = new QVBoxLayout(autoGroup);

    QLabel *noteLabel = new QLabel(
        "⚠️ Авто-оплата в разработке. Включите показ кнопки в Настройках.\n"
        "Здесь можно заранее настроить ключи для будущей интеграции.",
        autoGroup);
    noteLabel->setStyleSheet("color: #f0a000; font-size: 12px;");
    noteLabel->setWordWrap(true);
    autoLayout->addWidget(noteLabel);

    m_provTable = new QTableWidget(0, 5, autoGroup);
    m_provTable->setHorizontalHeaderLabels({
        "Вкл.", "Провайдер", "API Key / Terminal Key", "Password", "Описание"});
    m_provTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_provTable->setColumnWidth(0, 50);
    m_provTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_provTable->setColumnWidth(1, 120);
    m_provTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_provTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_provTable->setColumnWidth(3, 140);
    m_provTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_provTable->setColumnWidth(4, 200);
    autoLayout->addWidget(m_provTable);

    QPushButton *saveProvBtn = new QPushButton("💾 Сохранить провайдеры", autoGroup);
    saveProvBtn->setFixedWidth(200);
    autoLayout->addWidget(saveProvBtn, 0, Qt::AlignLeft);

    root->addWidget(autoGroup);
    root->addStretch();

    connect(saveLinkBtn,  &QPushButton::clicked, this, &PaymentsPage::saveManualLink);
    connect(saveProvBtn,  &QPushButton::clicked, this, &PaymentsPage::saveProviders);

    loadValues();
}

void PaymentsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadValues();
}

void PaymentsPage::loadValues()
{
    m_linkEdit->setText(m_config->manualPayLink());

    const QList<AutoProviderConfig> &providers = m_config->autoProviders();
    m_provTable->setRowCount(0);
    for (const AutoProviderConfig &p : providers) {
        int row = m_provTable->rowCount();
        m_provTable->insertRow(row);

        QCheckBox *cb = new QCheckBox(m_provTable);
        cb->setChecked(p.enabled);
        QWidget *cbWidget = new QWidget(m_provTable);
        QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
        cbLayout->addWidget(cb);
        cbLayout->setAlignment(Qt::AlignCenter);
        cbLayout->setContentsMargins(0,0,0,0);
        m_provTable->setCellWidget(row, 0, cbWidget);

        auto mkItem = [](const QString &t) {
            QTableWidgetItem *it = new QTableWidgetItem(t);
            return it;
        };
        m_provTable->setItem(row, 1, mkItem(p.displayName));
        m_provTable->setItem(row, 2, mkItem(p.apiKey.isEmpty() ? p.terminalKey : p.apiKey));
        m_provTable->setItem(row, 3, mkItem(p.password));
        m_provTable->setItem(row, 4, mkItem(p.description));
    }
}

void PaymentsPage::saveManualLink()
{
    m_config->setManualPayLink(m_linkEdit->text().trimmed());
    m_config->save();
    QMessageBox::information(this, "Сохранено", "Ссылка на оплату сохранена.");
}

void PaymentsPage::saveProviders()
{
    QList<AutoProviderConfig> providers = m_config->autoProviders();

    for (int row = 0; row < m_provTable->rowCount() && row < providers.size(); ++row) {
        QWidget *cbWidget = m_provTable->cellWidget(row, 0);
        if (cbWidget) {
            QCheckBox *cb = cbWidget->findChild<QCheckBox*>();
            if (cb) providers[row].enabled = cb->isChecked();
        }
        if (m_provTable->item(row, 2)) {
            // API Key and Terminal Key share the same UI column; save to both fields
            const QString keyValue = m_provTable->item(row, 2)->text().trimmed();
            providers[row].apiKey      = keyValue;
            providers[row].terminalKey = keyValue;
        }
        if (m_provTable->item(row, 3))
            providers[row].password = m_provTable->item(row, 3)->text().trimmed();
    }

    m_config->setAutoProviders(providers);
    m_config->save();
    QMessageBox::information(this, "Сохранено", "Настройки провайдеров сохранены.");
}
