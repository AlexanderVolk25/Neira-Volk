#pragma once

#include <QObject>
#include <QHash>
#include <QVariantMap>
#include <QTimer>

#include "configservice.h"
#include "telegramapiclient.h"
#include "orderservice.h"
#include "adminworkflow.h"

enum class UserState {
    Idle,
    SelectingPlan,
    SelectingPaymentMethod,
    SelectingAutoBank,
    WaitingReceipt,
    WaitingAdminReply
};

class BotEngine : public QObject
{
    Q_OBJECT
public:
    explicit BotEngine(ConfigService *config,
                       OrderService  *orders,
                       QObject       *parent = nullptr);

    void start();
    void stop();
    bool isRunning() const { return m_running; }

signals:
    void logMessage(const QString &text);
    void orderUpdated();

private slots:
    void poll();

private:
    // Update dispatchers
    void processUpdate(const QJsonObject &update);
    void processMessage(const QJsonObject &msg);
    void processCallbackQuery(const QJsonObject &cbq);

    // Command / flow handlers
    void handleStart(qint64 chatId, qint64 userId, const QString &username,
                     const QString &firstName);
    void handleTariffs(qint64 chatId);
    void handleBuy(qint64 chatId, qint64 userId);
    void handleSetup(qint64 chatId);
    void handleSupport(qint64 chatId);

    void sendMainMenu(qint64 chatId);

    // Plan / payment flow
    void handlePlanSelection(qint64 chatId, qint64 userId,
                              const QString &username, int planIndex);
    void handlePaymentMethodSelection(qint64 chatId, qint64 userId,
                                      const QString &choice);
    void handleAutoBankSelection(qint64 chatId, const QString &bank);
    void handleReceiptMessage(const QJsonObject &msg, qint64 chatId,
                              qint64 userId);

    // Admin flow
    void handleAdminCallback(qint64 adminChatId, const QString &callbackQueryId,
                              const QString &data);
    void handleAdminProxyReply(qint64 adminChatId, const QString &text);

    // User state helpers
    UserState userState(qint64 userId) const;
    void      setUserState(qint64 userId, UserState state);
    QVariantMap &userCtx(qint64 userId);

    // Build plans text
    QString buildPlansText() const;

    // Misc
    QString displayName(const QString &username, const QString &firstName) const;

    ConfigService     *m_config;
    OrderService      *m_orders;
    AdminWorkflow      m_adminWorkflow;
    TelegramApiClient  m_api;
    QTimer             m_pollTimer;
    QTimer             m_retryTimer;

    bool m_running = false;
    int  m_offset  = 0;
    int  m_retryDelay = 5000;

    QHash<qint64, QVariantMap> m_userCtx; // userId -> context map
};
