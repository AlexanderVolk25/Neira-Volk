#include "settingspage.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QShowEvent>
#include <QIntValidator>

SettingsPage::SettingsPage(ConfigService *config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel *title = new QLabel("⚙️ Settings", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    QGroupBox *group = new QGroupBox("Настройки бота", this);
    QFormLayout *form = new QFormLayout(group);
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight);

    m_tokenEdit = new QLineEdit(group);
    m_tokenEdit->setPlaceholderText("1234567890:AABBccdd...");
    m_tokenEdit->setEchoMode(QLineEdit::Password);

    m_adminIdEdit = new QLineEdit(group);
    m_adminIdEdit->setPlaceholderText("123456789");
    m_adminIdEdit->setValidator(new QIntValidator(group));

    m_supportEdit = new QLineEdit(group);
    m_supportEdit->setPlaceholderText("username (без @)");

    m_channelEdit = new QLineEdit(group);
    m_channelEdit->setPlaceholderText("@mychannel или -100xxxxxx");

    m_autoPayCheck = new QCheckBox("Показывать кнопку авто-оплаты пользователям", group);

    form->addRow("Bot Token:",      m_tokenEdit);
    form->addRow("Admin ID:",       m_adminIdEdit);
    form->addRow("Support @:",      m_supportEdit);
    form->addRow("Channel:",        m_channelEdit);
    form->addRow("Авто-оплата:",    m_autoPayCheck);

    root->addWidget(group);

    QPushButton *saveBtn = new QPushButton("💾 Сохранить", this);
    saveBtn->setFixedWidth(160);
    root->addWidget(saveBtn, 0, Qt::AlignLeft);

    root->addStretch();

    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::saveSettings);

    loadValues();
}

void SettingsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadValues();
}

void SettingsPage::loadValues()
{
    m_tokenEdit->setText(m_config->botToken());
    m_adminIdEdit->setText(QString::number(m_config->adminId()));
    m_supportEdit->setText(m_config->supportUsername());
    m_channelEdit->setText(m_config->channelUsername());
    m_autoPayCheck->setChecked(m_config->showAutoPayment());
}

void SettingsPage::saveSettings()
{
    m_config->setBotToken(m_tokenEdit->text().trimmed());
    m_config->setAdminId(m_adminIdEdit->text().toLongLong());
    m_config->setSupportUsername(m_supportEdit->text().trimmed());
    m_config->setChannelUsername(m_channelEdit->text().trimmed());
    m_config->setShowAutoPayment(m_autoPayCheck->isChecked());
    m_config->save();
    QMessageBox::information(this, "Сохранено", "Настройки успешно сохранены.");
}
