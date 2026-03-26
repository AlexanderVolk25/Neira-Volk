#include "messagespage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QMessageBox>

static QPlainTextEdit *makeTextEdit(QWidget *parent, int minHeight = 70)
{
    QPlainTextEdit *e = new QPlainTextEdit(parent);
    e->setMinimumHeight(minHeight);
    return e;
}

MessagesPage::MessagesPage(ConfigService *config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(24, 24, 24, 24);
    outerLayout->setSpacing(8);

    QLabel *title = new QLabel("💬 Messages", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    outerLayout->addWidget(title);

    // Scroll area for message templates
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget(scroll);
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setSpacing(12);

    auto addField = [&](const QString &label, QPlainTextEdit *&editor, const QString &placeholder = QString()) {
        QGroupBox *box = new QGroupBox(label, scrollContent);
        QVBoxLayout *bl = new QVBoxLayout(box);
        editor = makeTextEdit(box);
        if (!placeholder.isEmpty())
            editor->setPlaceholderText(placeholder);
        bl->addWidget(editor);
        contentLayout->addWidget(box);
    };

    addField("Приветствие (welcomeMsg)",        m_welcomeEdit);
    addField("Список тарифов (plansMsg) — {plans}", m_plansEdit, "Используйте {plans} для подстановки");
    addField("Как настроить (howToSetupMsg)",   m_setupEdit);
    addField("Инструкция к оплате (paymentInstructionMsg) — {plan}, {price}, {order_id}",
             m_payInstrEdit, "Используйте {order_id}, {plan}, {price}");
    addField("Уведомление админу (adminNotifyMsg) — {order_id}, {user}, {plan}, {price}",
             m_adminNotifyEdit);
    addField("Чек получен (checkReceivedMsg)",  m_checkReceivedEdit);
    addField("Заказ подтверждён (orderConfirmedMsg) — {proxy_data}", m_confirmedEdit);
    addField("Заказ отклонён (orderRejectedMsg)",  m_rejectedEdit);

    scroll->setWidget(scrollContent);
    outerLayout->addWidget(scroll, 1);

    QPushButton *saveBtn = new QPushButton("💾 Сохранить шаблоны", this);
    saveBtn->setFixedWidth(200);
    outerLayout->addWidget(saveBtn, 0, Qt::AlignLeft);

    connect(saveBtn, &QPushButton::clicked, this, &MessagesPage::saveMessages);

    loadValues();
}

void MessagesPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadValues();
}

void MessagesPage::loadValues()
{
    m_welcomeEdit->setPlainText(m_config->welcomeMsg());
    m_plansEdit->setPlainText(m_config->plansMsg());
    m_setupEdit->setPlainText(m_config->howToSetupMsg());
    m_payInstrEdit->setPlainText(m_config->paymentInstructionMsg());
    m_adminNotifyEdit->setPlainText(m_config->adminNotifyMsg());
    m_checkReceivedEdit->setPlainText(m_config->checkReceivedMsg());
    m_confirmedEdit->setPlainText(m_config->orderConfirmedMsg());
    m_rejectedEdit->setPlainText(m_config->orderRejectedMsg());
}

void MessagesPage::saveMessages()
{
    m_config->setWelcomeMsg(m_welcomeEdit->toPlainText());
    m_config->setPlansMsg(m_plansEdit->toPlainText());
    m_config->setHowToSetupMsg(m_setupEdit->toPlainText());
    m_config->setPaymentInstructionMsg(m_payInstrEdit->toPlainText());
    m_config->setAdminNotifyMsg(m_adminNotifyEdit->toPlainText());
    m_config->setCheckReceivedMsg(m_checkReceivedEdit->toPlainText());
    m_config->setOrderConfirmedMsg(m_confirmedEdit->toPlainText());
    m_config->setOrderRejectedMsg(m_rejectedEdit->toPlainText());
    m_config->save();
    QMessageBox::information(this, "Сохранено", "Шаблоны сообщений сохранены.");
}
