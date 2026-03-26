#include "telegramapiclient.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

TelegramApiClient::TelegramApiClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished, this,
            [this](QNetworkReply *reply) {
                if (reply->error() != QNetworkReply::NoError)
                    emit networkError(reply->errorString());
            });
}

void TelegramApiClient::setToken(const QString &token)
{
    m_token   = token;
    m_baseUrl = QString("https://api.telegram.org/bot%1/").arg(token);
}

QNetworkReply *TelegramApiClient::getUpdates(int offset, int timeout)
{
    QJsonObject params;
    params["offset"]  = offset;
    params["timeout"] = timeout;
    return post("getUpdates", params);
}

QNetworkReply *TelegramApiClient::sendMessage(qint64 chatId,
                                               const QString &text,
                                               const QString &replyMarkup)
{
    QJsonObject params;
    params["chat_id"]    = chatId;
    params["text"]       = text;
    params["parse_mode"] = "HTML";
    if (!replyMarkup.isEmpty())
        params["reply_markup"] = replyMarkup;
    return post("sendMessage", params);
}

QNetworkReply *TelegramApiClient::sendMessageWithInlineKeyboard(
    qint64 chatId,
    const QString &text,
    const QVector<QVector<QPair<QString,QString>>> &buttons)
{
    QJsonObject params;
    params["chat_id"]      = chatId;
    params["text"]         = text;
    params["parse_mode"]   = "HTML";
    params["reply_markup"] = buildInlineKeyboardJson(buttons);
    return post("sendMessage", params);
}

QNetworkReply *TelegramApiClient::sendPhoto(qint64 chatId,
                                             const QString &fileId,
                                             const QString &caption)
{
    QJsonObject params;
    params["chat_id"]    = chatId;
    params["photo"]      = fileId;
    params["parse_mode"] = "HTML";
    if (!caption.isEmpty())
        params["caption"] = caption;
    return post("sendPhoto", params);
}

QNetworkReply *TelegramApiClient::sendDocument(qint64 chatId,
                                                const QString &fileId,
                                                const QString &caption)
{
    QJsonObject params;
    params["chat_id"]    = chatId;
    params["document"]   = fileId;
    params["parse_mode"] = "HTML";
    if (!caption.isEmpty())
        params["caption"] = caption;
    return post("sendDocument", params);
}

QNetworkReply *TelegramApiClient::forwardMessage(qint64 chatId,
                                                  qint64 fromChatId,
                                                  int messageId)
{
    QJsonObject params;
    params["chat_id"]      = chatId;
    params["from_chat_id"] = fromChatId;
    params["message_id"]   = messageId;
    return post("forwardMessage", params);
}

QNetworkReply *TelegramApiClient::answerCallbackQuery(const QString &callbackQueryId,
                                                       const QString &text)
{
    QJsonObject params;
    params["callback_query_id"] = callbackQueryId;
    if (!text.isEmpty())
        params["text"] = text;
    return post("answerCallbackQuery", params);
}

QNetworkReply *TelegramApiClient::sendToChannel(const QString &channelId,
                                                 const QString &text)
{
    QJsonObject params;
    params["chat_id"]    = channelId;
    params["text"]       = text;
    params["parse_mode"] = "HTML";
    return post("sendMessage", params);
}

// ---------- private helpers ----------

QNetworkReply *TelegramApiClient::post(const QString &method,
                                        const QJsonObject &params)
{
    QNetworkRequest req;
    req.setUrl(QUrl(m_baseUrl + method));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(params).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    return reply;
}

QString TelegramApiClient::buildInlineKeyboardJson(
    const QVector<QVector<QPair<QString,QString>>> &buttons)
{
    QJsonArray rows;
    for (const auto &row : buttons) {
        QJsonArray rowArr;
        for (const auto &btn : row) {
            QJsonObject b;
            b["text"] = btn.first;
            const QString &data = btn.second;
            // Detect URL buttons (start with http:// or https://)
            if (data.startsWith("http://") || data.startsWith("https://"))
                b["url"] = data;
            else
                b["callback_data"] = data;
            rowArr.append(b);
        }
        rows.append(rowArr);
    }
    QJsonObject markup;
    markup["inline_keyboard"] = rows;
    return QString::fromUtf8(QJsonDocument(markup).toJson(QJsonDocument::Compact));
}
