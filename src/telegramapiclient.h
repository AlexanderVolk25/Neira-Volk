#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class TelegramApiClient : public QObject
{
    Q_OBJECT
public:
    explicit TelegramApiClient(QObject *parent = nullptr);

    void setToken(const QString &token);

    QNetworkReply *getUpdates(int offset, int timeout = 25);
    QNetworkReply *sendMessage(qint64 chatId, const QString &text,
                               const QString &replyMarkup = QString());
    QNetworkReply *sendMessageWithInlineKeyboard(
        qint64 chatId,
        const QString &text,
        const QVector<QVector<QPair<QString,QString>>> &buttons);
    QNetworkReply *sendPhoto(qint64 chatId, const QString &fileId,
                             const QString &caption = QString());
    QNetworkReply *sendDocument(qint64 chatId, const QString &fileId,
                                const QString &caption = QString());
    QNetworkReply *forwardMessage(qint64 chatId, qint64 fromChatId, int messageId);
    QNetworkReply *answerCallbackQuery(const QString &callbackQueryId,
                                       const QString &text = QString());
    QNetworkReply *sendToChannel(const QString &channelId, const QString &text);

signals:
    void networkError(const QString &errorText);

private:
    QNetworkReply *post(const QString &method, const QJsonObject &params);
    QNetworkReply *get(const QString &method, const QJsonObject &params);
    QString buildInlineKeyboardJson(
        const QVector<QVector<QPair<QString,QString>>> &buttons);

    QNetworkAccessManager *m_nam;
    QString m_token;
    QString m_baseUrl;
};
