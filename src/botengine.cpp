#include "botengine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QDateTime>
#include <QDebug>

// Context map keys
static const char *K_STATE     = "state";
static const char *K_PLAN_IDX  = "planIndex";
static const char *K_ORDER_ID  = "orderId";
static const char *K_CHAT_ID   = "chatId";
static const char *K_USERNAME  = "username";

BotEngine::BotEngine(ConfigService *config, OrderService *orders, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_orders(orders)
    , m_adminWorkflow(this)
    , m_api(this)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &BotEngine::poll);

    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, [this]() {
        if (m_running)
            poll();
    });
}

void BotEngine::start()
{
    if (m_running) return;
    m_api.setToken(m_config->botToken());
    m_running = true;
    m_offset  = 0;
    emit logMessage("🟢 Бот запущен");
    poll();
}

void BotEngine::stop()
{
    m_running = false;
    m_pollTimer.stop();
    m_retryTimer.stop();
    emit logMessage("🔴 Бот остановлен");
}

void BotEngine::poll()
{
    if (!m_running) return;

    QNetworkReply *reply = m_api.getUpdates(m_offset, 25);
    if (!reply) return;

    // Disconnect deleteLater set in TelegramApiClient so we control lifetime here
    disconnect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (!m_running) return;

        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage(QString("⚠️ Ошибка сети: %1").arg(reply->errorString()));
            m_retryTimer.start(m_retryDelay);
            m_retryDelay = qMin(m_retryDelay * 2, 60000);
            return;
        }

        m_retryDelay = 5000;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            m_retryTimer.start(m_retryDelay);
            return;
        }

        QJsonObject root = doc.object();
        if (!root.value("ok").toBool()) {
            emit logMessage("⚠️ Telegram API вернул ok=false");
            m_retryTimer.start(m_retryDelay);
            return;
        }

        QJsonArray updates = root.value("result").toArray();
        for (const QJsonValue &v : updates) {
            QJsonObject upd = v.toObject();
            int updateId = upd.value("update_id").toInt();
            if (updateId >= m_offset)
                m_offset = updateId + 1;
            processUpdate(upd);
        }

        // Schedule next poll immediately
        if (m_running)
            QTimer::singleShot(0, this, &BotEngine::poll);
    });
}

// ----------------------------------------------------------------
//  Update dispatch
// ----------------------------------------------------------------

void BotEngine::processUpdate(const QJsonObject &update)
{
    if (update.contains("message"))
        processMessage(update.value("message").toObject());
    else if (update.contains("callback_query"))
        processCallbackQuery(update.value("callback_query").toObject());
}

void BotEngine::processMessage(const QJsonObject &msg)
{
    const QJsonObject fromObj = msg.value("from").toObject();
    const qint64 userId   = fromObj.value("id").toVariant().toLongLong();
    const qint64 chatId   = msg.value("chat").toObject().value("id").toVariant().toLongLong();
    const QString username  = fromObj.value("username").toString();
    const QString firstName = fromObj.value("first_name").toString();
    const QString text      = msg.value("text").toString();

    m_orders->upsertUser(userId, username, firstName);

    // ----- Admin awaiting proxy data reply -----
    if (userId == m_config->adminId() && m_adminWorkflow.hasPendingReply(userId)) {
        if (!text.isEmpty()) {
            handleAdminProxyReply(chatId, text);
            return;
        }
    }

    // ----- Commands -----
    if (text == "/start") {
        handleStart(chatId, userId, username, firstName);
        return;
    }

    // ----- Main menu text buttons -----
    if (text == "Тарифы") {
        handleTariffs(chatId);
        return;
    }
    if (text == "Купить") {
        handleBuy(chatId, userId);
        return;
    }
    if (text == "Как настроить") {
        handleSetup(chatId);
        return;
    }
    if (text == "Поддержка") {
        handleSupport(chatId);
        return;
    }

    // ----- State machine -----
    UserState state = userState(userId);

    if (state == UserState::WaitingReceipt) {
        handleReceiptMessage(msg, chatId, userId);
        return;
    }

    // Default
    sendMainMenu(chatId);
}

void BotEngine::processCallbackQuery(const QJsonObject &cbq)
{
    const QString cbqId   = cbq.value("id").toString();
    const QJsonObject fromObj = cbq.value("from").toObject();
    const qint64 userId   = fromObj.value("id").toVariant().toLongLong();
    const qint64 chatId   = cbq.value("message").toObject()
                                .value("chat").toObject()
                                .value("id").toVariant().toLongLong();
    const QString username  = fromObj.value("username").toString();
    const QString data      = cbq.value("data").toString();

    m_api.answerCallbackQuery(cbqId);

    // Admin callbacks
    if (userId == m_config->adminId() &&
        (data.startsWith("confirm_") || data.startsWith("reject_") ||
         data.startsWith("clarify_"))) {
        handleAdminCallback(chatId, cbqId, data);
        return;
    }

    // Plan selection  e.g. "plan_0"
    if (data.startsWith("plan_")) {
        int idx = data.mid(5).toInt();
        handlePlanSelection(chatId, userId, username, idx);
        return;
    }

    // Payment method
    if (data == "pay_manual" || data == "pay_auto" || data == "pay_back") {
        handlePaymentMethodSelection(chatId, userId, data);
        return;
    }

    // Auto bank selection
    if (data.startsWith("bank_")) {
        handleAutoBankSelection(chatId, data.mid(5));
        return;
    }

    // Send receipt button
    if (data.startsWith("send_receipt_")) {
        int orderId = data.mid(13).toInt();
        userCtx(userId)[K_ORDER_ID] = orderId;
        setUserState(userId, UserState::WaitingReceipt);
        m_api.sendMessage(chatId, "📎 Пришлите чек — фото или PDF-документ.");
        return;
    }

    // Cancel order
    if (data.startsWith("cancel_order_")) {
        int orderId = data.mid(13).toInt();
        m_orders->updateOrderStatus(orderId, "rejected");
        setUserState(userId, UserState::Idle);
        m_api.sendMessage(chatId, "❌ Заказ отменён.");
        emit logMessage(QString("Заказ #%1 отменён пользователем %2").arg(orderId).arg(username));
        emit orderUpdated();
        return;
    }
}

// ----------------------------------------------------------------
//  Handlers
// ----------------------------------------------------------------

void BotEngine::handleStart(qint64 chatId, qint64 userId,
                             const QString &username, const QString &firstName)
{
    setUserState(userId, UserState::Idle);
    const QString name = displayName(username, firstName);
    QString msg = m_config->welcomeMsg();
    emit logMessage(QString("👤 /start от %1 (id=%2)").arg(name).arg(userId));

    // Send welcome + main menu keyboard
    QJsonObject replyMarkup;
    QJsonArray keyboard;
    QJsonArray row1, row2;
    row1.append(QJsonObject{{"text", "Тарифы"}});
    row1.append(QJsonObject{{"text", "Купить"}});
    row2.append(QJsonObject{{"text", "Как настроить"}});
    row2.append(QJsonObject{{"text", "Поддержка"}});
    keyboard.append(row1);
    keyboard.append(row2);
    replyMarkup["keyboard"]          = keyboard;
    replyMarkup["resize_keyboard"]   = true;
    replyMarkup["one_time_keyboard"] = false;

    QJsonObject params;
    params["chat_id"]      = chatId;
    params["text"]         = msg;
    params["parse_mode"]   = "HTML";
    params["reply_markup"] = QString::fromUtf8(
        QJsonDocument(replyMarkup).toJson(QJsonDocument::Compact));

    // Use sendMessage directly via the API
    m_api.sendMessage(chatId, msg,
        QString::fromUtf8(QJsonDocument(replyMarkup).toJson(QJsonDocument::Compact)));
}

void BotEngine::sendMainMenu(qint64 chatId)
{
    QJsonObject replyMarkup;
    QJsonArray keyboard;
    QJsonArray row1, row2;
    row1.append(QJsonObject{{"text", "Тарифы"}});
    row1.append(QJsonObject{{"text", "Купить"}});
    row2.append(QJsonObject{{"text", "Как настроить"}});
    row2.append(QJsonObject{{"text", "Поддержка"}});
    keyboard.append(row1);
    keyboard.append(row2);
    replyMarkup["keyboard"]        = keyboard;
    replyMarkup["resize_keyboard"] = true;

    m_api.sendMessage(chatId, "Выберите действие:",
        QString::fromUtf8(QJsonDocument(replyMarkup).toJson(QJsonDocument::Compact)));
}

void BotEngine::handleTariffs(qint64 chatId)
{
    QString text = m_config->plansMsg();
    text.replace("{plans}", buildPlansText());
    m_api.sendMessage(chatId, text);
}

void BotEngine::handleBuy(qint64 chatId, qint64 userId)
{
    const QList<PlanConfig> &plans = m_config->plans();
    if (plans.isEmpty()) {
        m_api.sendMessage(chatId, "Тарифы не настроены. Обратитесь к администратору.");
        return;
    }

    setUserState(userId, UserState::SelectingPlan);

    QVector<QVector<QPair<QString,QString>>> buttons;
    for (int i = 0; i < plans.size(); ++i) {
        const PlanConfig &p = plans[i];
        QString label = QString("%1 — %2 ₽").arg(p.name).arg(p.price);
        buttons.append({{label, QString("plan_%1").arg(i)}});
    }
    buttons.append({{"🔙 Назад", "pay_back"}});

    m_api.sendMessageWithInlineKeyboard(chatId, "💳 Выберите тариф:", buttons);
}

void BotEngine::handleSetup(qint64 chatId)
{
    m_api.sendMessage(chatId, m_config->howToSetupMsg());
}

void BotEngine::handleSupport(qint64 chatId)
{
    const QString sup = m_config->supportUsername();
    if (!sup.isEmpty()) {
        QString handle = sup.startsWith("@") ? sup : "@" + sup;
        m_api.sendMessage(chatId,
            QString("💬 Служба поддержки: %1").arg(handle));
    } else {
        m_api.sendMessage(chatId,
            QString("💬 Обратитесь к администратору: @%1")
                .arg(m_config->supportUsername().isEmpty() ? "admin" : m_config->supportUsername()));
    }
}

void BotEngine::handlePlanSelection(qint64 chatId, qint64 userId,
                                     const QString &username, int planIndex)
{
    const QList<PlanConfig> &plans = m_config->plans();
    if (planIndex < 0 || planIndex >= plans.size()) {
        m_api.sendMessage(chatId, "Неверный выбор тарифа.");
        return;
    }

    userCtx(userId)[K_PLAN_IDX] = planIndex;
    userCtx(userId)[K_USERNAME] = username;
    setUserState(userId, UserState::SelectingPaymentMethod);

    const PlanConfig &plan = plans[planIndex];

    QVector<QVector<QPair<QString,QString>>> buttons;
    buttons.append({{"💵 Оплатить вручную (чек обязателен)", "pay_manual"}});
    if (m_config->showAutoPayment())
        buttons.append({{"🤖 Оплатить через бота (Авто)", "pay_auto"}});
    buttons.append({{"🔙 Назад", "pay_back"}});

    m_api.sendMessageWithInlineKeyboard(
        chatId,
        QString("Тариф: <b>%1</b> — %2 ₽\n\nВыберите способ оплаты:").arg(plan.name).arg(plan.price),
        buttons);
}

void BotEngine::handlePaymentMethodSelection(qint64 chatId, qint64 userId,
                                              const QString &choice)
{
    if (choice == "pay_back") {
        setUserState(userId, UserState::Idle);
        handleBuy(chatId, userId);
        return;
    }

    int planIndex = userCtx(userId).value(K_PLAN_IDX, -1).toInt();
    const QList<PlanConfig> &plans = m_config->plans();
    if (planIndex < 0 || planIndex >= plans.size()) {
        m_api.sendMessage(chatId, "Ошибка: тариф не выбран. Начните заново.");
        setUserState(userId, UserState::Idle);
        return;
    }

    const PlanConfig &plan = plans[planIndex];
    const QString username = userCtx(userId).value(K_USERNAME).toString();

    if (choice == "pay_manual") {
        // Create order
        int orderId = m_orders->createOrder(userId, username, chatId,
                                             plan.name, plan.price);
        if (orderId < 0) {
            m_api.sendMessage(chatId, "Ошибка при создании заказа. Попробуйте позже.");
            return;
        }
        userCtx(userId)[K_ORDER_ID] = orderId;
        setUserState(userId, UserState::WaitingReceipt);

        QString instrText = m_config->paymentInstructionMsg();
        instrText.replace("{order_id}", QString::number(orderId));
        instrText.replace("{plan}",     plan.name);
        instrText.replace("{price}",    QString::number(plan.price));

        const QString payLink = m_config->manualPayLink();

        QVector<QVector<QPair<QString,QString>>> buttons;
        if (!payLink.isEmpty())
            buttons.append({{"🔗 Открыть ссылку на оплату", payLink}});
        buttons.append({{"📤 Отправить чек", QString("send_receipt_%1").arg(orderId)}});
        buttons.append({{"❌ Отменить", QString("cancel_order_%1").arg(orderId)}});

        m_api.sendMessageWithInlineKeyboard(chatId, instrText, buttons);

        emit logMessage(QString("📦 Заказ #%1 создан: %2, %3 ₽").arg(orderId).arg(plan.name).arg(plan.price));
        emit orderUpdated();

    } else if (choice == "pay_auto") {
        setUserState(userId, UserState::SelectingAutoBank);

        const QList<AutoProviderConfig> &providers = m_config->autoProviders();
        QVector<QVector<QPair<QString,QString>>> buttons;
        bool anyEnabled = false;
        for (const AutoProviderConfig &p : providers) {
            if (p.enabled) {
                buttons.append({{p.displayName, QString("bank_%1").arg(p.name)}});
                anyEnabled = true;
            }
        }

        if (!anyEnabled) {
            m_api.sendMessage(chatId, "⚠️ Авто-оплата временно недоступна.");
            setUserState(userId, UserState::SelectingPaymentMethod);
            return;
        }

        buttons.append({{"🔙 Назад", "pay_back"}});
        m_api.sendMessageWithInlineKeyboard(chatId, "🏦 Выберите банк:", buttons);
    }
}

void BotEngine::handleAutoBankSelection(qint64 chatId, const QString &bank)
{
    // Skeleton: no real API integration
    QString bankName = bank;
    const QList<AutoProviderConfig> &providers = m_config->autoProviders();
    for (const AutoProviderConfig &p : providers) {
        if (p.name == bank) {
            bankName = p.displayName;
            break;
        }
    }
    m_api.sendMessage(chatId,
        QString("ℹ️ Оплата через %1: функция в разработке — скоро будет доступна.").arg(bankName));
}

void BotEngine::handleReceiptMessage(const QJsonObject &msg, qint64 chatId,
                                      qint64 userId)
{
    const QString text = msg.value("text").toString();
    int orderId = userCtx(userId).value(K_ORDER_ID, -1).toInt();

    // Must be a photo or document
    bool hasPhoto = msg.contains("photo");
    bool hasDoc   = msg.contains("document");

    if (!hasPhoto && !hasDoc) {
        m_api.sendMessage(chatId,
            "⚠️ Чек обязателен. Пришлите фото или PDF-документ.");
        return;
    }

    // Get file_id
    QString fileId;
    bool isPhoto = false;
    if (hasPhoto) {
        QJsonArray photos = msg.value("photo").toArray();
        if (!photos.isEmpty())
            fileId = photos.last().toObject().value("file_id").toString();
        isPhoto = true;
    } else {
        fileId = msg.value("document").toObject().value("file_id").toString();
    }

    // Notify user
    m_api.sendMessage(chatId, m_config->checkReceivedMsg());

    // Save receipt
    if (orderId > 0)
        m_orders->setReceiptFileId(orderId, fileId);

    Order order = m_orders->getOrder(orderId);

    // Notify admin
    qint64 adminId = m_config->adminId();
    QString username = userCtx(userId).value(K_USERNAME).toString();
    QString adminText = m_config->adminNotifyMsg();
    adminText.replace("{order_id}", QString::number(orderId));
    adminText.replace("{user}",     username.isEmpty() ? QString::number(userId) : "@" + username);
    adminText.replace("{plan}",     order.planName);
    adminText.replace("{price}",    QString::number(order.planPrice));

    QVector<QVector<QPair<QString,QString>>> adminButtons = {
        {{"✅ Подтвердить",       QString("confirm_%1").arg(orderId)},
         {"❌ Отклонить",         QString("reject_%1").arg(orderId)}},
        {{"💬 Запросить уточнение", QString("clarify_%1").arg(orderId)}}
    };

    // Send text notification with buttons
    QNetworkReply *notifReply = m_api.sendMessageWithInlineKeyboard(adminId, adminText, adminButtons);
    // We need the message_id of admin notification to store it
    // Use a lambda on a separate reply
    disconnect(notifReply, &QNetworkReply::finished, notifReply, &QNetworkReply::deleteLater);
    connect(notifReply, &QNetworkReply::finished, this, [this, notifReply, orderId]() {
        notifReply->deleteLater();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(notifReply->readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            int msgId = doc.object()
                .value("result").toObject()
                .value("message_id").toInt();
            if (msgId > 0)
                m_orders->setAdminMsgId(orderId, msgId);
        }
    });

    // Forward the receipt
    qint64 fromChatId = chatId;
    int    receiptMsgId = msg.value("message_id").toInt();
    m_api.forwardMessage(adminId, fromChatId, receiptMsgId);

    setUserState(userId, UserState::Idle);
    m_orders->updateOrderStatus(orderId, "pending");

    emit logMessage(QString("📎 Чек получен для заказа #%1").arg(orderId));
    emit orderUpdated();
}

void BotEngine::handleAdminCallback(qint64 adminChatId,
                                     const QString &callbackQueryId,
                                     const QString &data)
{
    auto parseOrderId = [](const QString &d, const QString &prefix) -> int {
        return d.mid(prefix.size()).toInt();
    };

    if (data.startsWith("confirm_")) {
        int orderId = parseOrderId(data, "confirm_");
        m_adminWorkflow.setPendingReply(adminChatId, orderId);
        m_api.sendMessage(adminChatId,
            QString("📝 Заказ #%1 — ответьте на это сообщение данными прокси, "
                    "я перешлю их клиенту.").arg(orderId));
        emit logMessage(QString("✅ Админ подтверждает заказ #%1").arg(orderId));

    } else if (data.startsWith("reject_")) {
        int orderId = parseOrderId(data, "reject_");
        m_orders->updateOrderStatus(orderId, "rejected");
        Order order = m_orders->getOrder(orderId);
        if (order.chatId > 0) {
            m_api.sendMessage(order.chatId, m_config->orderRejectedMsg());
        }
        m_api.sendMessage(adminChatId,
            QString("❌ Заказ #%1 отклонён.").arg(orderId));
        emit logMessage(QString("❌ Заказ #%1 отклонён").arg(orderId));
        emit orderUpdated();

    } else if (data.startsWith("clarify_")) {
        int orderId = parseOrderId(data, "clarify_");
        Order order = m_orders->getOrder(orderId);
        if (order.chatId > 0) {
            m_api.sendMessage(order.chatId,
                "❓ Администратор запрашивает уточнение по вашему заказу. "
                "Пожалуйста, напишите дополнительные сведения.");
        }
        m_api.sendMessage(adminChatId,
            QString("💬 Запрос уточнения отправлен клиенту (заказ #%1).").arg(orderId));
    }
}

void BotEngine::handleAdminProxyReply(qint64 adminChatId, const QString &text)
{
    int orderId = m_adminWorkflow.getPendingOrderId(adminChatId);
    m_adminWorkflow.clearPendingReply(adminChatId);

    if (orderId < 0) return;

    m_orders->setProxyData(orderId, text);
    m_orders->updateOrderStatus(orderId, "delivered");

    Order order = m_orders->getOrder(orderId);
    if (order.chatId > 0) {
        QString confirmMsg = m_config->orderConfirmedMsg();
        confirmMsg.replace("{proxy_data}", text);
        m_api.sendMessage(order.chatId, confirmMsg);
    }

    m_api.sendMessage(adminChatId,
        QString("✅ Данные отправлены клиенту. Заказ #%1 выполнен.").arg(orderId));

    emit logMessage(QString("🎉 Заказ #%1 выполнен, данные отправлены").arg(orderId));
    emit orderUpdated();
}

// ----------------------------------------------------------------
//  Helpers
// ----------------------------------------------------------------

UserState BotEngine::userState(qint64 userId) const
{
    if (!m_userCtx.contains(userId)) return UserState::Idle;
    return static_cast<UserState>(m_userCtx[userId].value(K_STATE, 0).toInt());
}

void BotEngine::setUserState(qint64 userId, UserState state)
{
    m_userCtx[userId][K_STATE] = static_cast<int>(state);
}

QVariantMap &BotEngine::userCtx(qint64 userId)
{
    return m_userCtx[userId];
}

QString BotEngine::buildPlansText() const
{
    QString result;
    const QList<PlanConfig> &plans = m_config->plans();
    for (const PlanConfig &p : plans) {
        result += QString("• <b>%1</b> — %2 ₽\n").arg(p.name).arg(p.price);
    }
    return result.trimmed();
}

QString BotEngine::displayName(const QString &username,
                                const QString &firstName) const
{
    if (!username.isEmpty()) return "@" + username;
    if (!firstName.isEmpty()) return firstName;
    return "Unknown";
}
