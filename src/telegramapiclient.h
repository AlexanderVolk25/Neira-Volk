#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QVector>
#include <QPair>

class TelegramApiClient : public QObject
{
    Q_OBJECT
public:
    explicit TelegramApiClient(QObject *parent = nullptr);

    void setToken(const QString &token);

    QNetworkReply *getUpdates(int offset, int timeout = 25);
    QNetworkReply *sendMessage(qint64 chatId, const QString &text,
                               const QJsonObject &replyMarkup = QJsonObject());
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

    static QJsonObject buildInlineKeyboardMarkup(
        const QVector<QVector<QPair<QString,QString>>> &buttons);

signals:
    void networkError(const QString &errorText);

private:
    QNetworkReply *post(const QString &method, const QJsonObject &params);

    QNetworkAccessManager *m_nam;
    QString m_token;
    QString m_baseUrl;
};
