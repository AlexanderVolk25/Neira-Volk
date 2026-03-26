#include "channelpage.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QShowEvent>
#include <QNetworkReply>

ChannelPage::ChannelPage(ConfigService *config, BotEngine *engine, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
    , m_engine(engine)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel *title = new QLabel("📢 Channel", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #6ab0ff;");
    root->addWidget(title);

    QGroupBox *group = new QGroupBox("Публикация в канал", this);
    QVBoxLayout *gl  = new QVBoxLayout(group);

    QLabel *hint = new QLabel("Убедитесь, что бот добавлен в канал как администратор.", group);
    hint->setStyleSheet("color: #a0b4d0;");
    hint->setWordWrap(true);
    gl->addWidget(hint);

    m_postEdit = new QPlainTextEdit(group);
    m_postEdit->setMinimumHeight(160);
    m_postEdit->setPlaceholderText("Введите текст публикации...");
    gl->addWidget(m_postEdit);

    QPushButton *publishBtn = new QPushButton("📤 Опубликовать в канал", group);
    publishBtn->setFixedWidth(220);
    gl->addWidget(publishBtn, 0, Qt::AlignLeft);

    m_statusLabel = new QLabel("", group);
    m_statusLabel->setWordWrap(true);
    gl->addWidget(m_statusLabel);

    root->addWidget(group);
    root->addStretch();

    connect(publishBtn, &QPushButton::clicked, this, &ChannelPage::publishPost);
}

void ChannelPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Reload channel post text if set
    if (m_postEdit->toPlainText().isEmpty())
        m_postEdit->setPlainText(m_config->channelPostText());
}

void ChannelPage::publishPost()
{
    const QString channel = m_config->channelUsername();
    if (channel.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        m_statusLabel->setText("❌ Укажите username/ID канала в Настройках.");
        return;
    }

    const QString text = m_postEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        m_statusLabel->setText("❌ Текст публикации не может быть пустым.");
        return;
    }

    if (!m_engine->isRunning()) {
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        m_statusLabel->setText("❌ Бот не запущен. Запустите его на Dashboard.");
        return;
    }

    // Save post text to config
    m_config->setChannelPostText(text);
    m_config->save();

    m_statusLabel->setStyleSheet("color: #a0b4d0;");
    m_statusLabel->setText("⏳ Отправляем...");

    // We need to call sendToChannel via TelegramApiClient directly.
    // BotEngine owns a private TelegramApiClient; expose publish via a signal/slot
    // or create a thin client here. We use a simple approach: re-use BotEngine's
    // publishToChannel slot (added below via signal).
    emit m_engine->logMessage(QString("📢 Публикация в канал %1").arg(channel));

    // Since BotEngine doesn't expose a direct publishToChannel method on its
    // public API, we create a one-shot TelegramApiClient here for publishing.
    // This is safe because the token & channel are known.
    TelegramApiClient *tempClient = new TelegramApiClient(this);
    tempClient->setToken(m_config->botToken());
    QNetworkReply *reply = tempClient->sendToChannel(channel, text);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, tempClient]() {
                if (reply->error() == QNetworkReply::NoError) {
                    m_statusLabel->setStyleSheet("color: #4caf50;");
                    m_statusLabel->setText("✅ Опубликовано успешно!");
                } else {
                    m_statusLabel->setStyleSheet("color: #e74c3c;");
                    m_statusLabel->setText(
                        QString("❌ Ошибка: %1").arg(reply->errorString()));
                }
                tempClient->deleteLater();
            });
}
