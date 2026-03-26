#include "botengine.h"

#include <sstream>
#include <chrono>
#include "utils.h"

// ---------- constructor / destructor ----------

BotEngine::BotEngine(ConfigService* config, OrderService* orders)
    : m_config(config), m_orders(orders)
{
}

BotEngine::~BotEngine()
{
    stop();
}

void BotEngine::start()
{
    if (m_running.load()) return;
    m_api.setToken(m_config->botToken());
    m_running.store(true);
    m_thread = std::thread(&BotEngine::runLoop, this);
    log("🟢 Бот запущен");
}

void BotEngine::stop()
{
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_thread.joinable())
        m_thread.join();
    log("🔴 Бот остановлен");
}

// ---------- main poll loop (worker thread) ----------

void BotEngine::runLoop()
{
    int retryDelaySec = 5;

    while (m_running.load()) {
        ApiResponse resp = m_api.getUpdates(m_offset, 25);

        if (!resp.ok) {
            log("⚠️ Ошибка сети: " + resp.raw.substr(0, 120));
            // exponential back-off
            for (int i = 0; i < retryDelaySec * 10 && m_running.load(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retryDelaySec = std::min(retryDelaySec * 2, 60);
            continue;
        }
        retryDelaySec = 5;

        if (!resp.result.is_array()) continue;

        for (const auto& update : resp.result) {
            processUpdate(update);
            int uid = update.value("update_id", 0);
            if (uid >= m_offset) m_offset = uid + 1;
        }
    }
}

// ---------- update dispatching ----------

void BotEngine::processUpdate(const nlohmann::json& update)
{
    if (update.contains("message"))
        processMessage(update["message"]);
    else if (update.contains("callback_query"))
        processCallbackQuery(update["callback_query"]);
}

void BotEngine::processMessage(const nlohmann::json& msg)
{
    int64_t chatId    = msg.contains("chat") ? msg["chat"].value("id", (int64_t)0) : 0;
    int64_t userId    = msg.contains("from") ? msg["from"].value("id", (int64_t)0) : 0;
    std::string uname = msg.contains("from") ? msg["from"].value("username", std::string{}) : "";
    std::string fname = msg.contains("from") ? msg["from"].value("first_name", std::string{}) : "";
    std::string text  = msg.value("text", std::string{});

    if (chatId == 0 || userId == 0) return;

    m_orders->upsertUser(userId, uname, fname);

    // Admin check: is this the admin sending proxy data?
    if (userId == m_config->adminId() && m_adminWorkflow.hasPendingReply(chatId)) {
        handleAdminProxyReply(chatId, text);
        return;
    }

    UserContext& ctx = userCtx(userId);

    // Command routing
    if (text == "/start") {
        ctx.state = UserState::Idle;
        handleStart(chatId, userId, uname, fname);
        return;
    }

    // State-machine routing
    switch (ctx.state) {
    case UserState::SelectingPlan:
        // handled via inline keyboard callbacks
        break;
    case UserState::SelectingPaymentMethod:
        break;
    case UserState::WaitingReceipt:
        handleReceiptMessage(msg, chatId, userId);
        return;
    default:
        break;
    }

    // Button text matching (reply keyboard)
    if (text == "📋 Тарифы")       { handleTariffs(chatId); return; }
    if (text == "🛒 Купить")        { handleBuy(chatId, userId); return; }
    if (text == "⚙️ Как настроить") { handleSetup(chatId); return; }
    if (text == "🆘 Поддержка")     { handleSupport(chatId); return; }
}

void BotEngine::processCallbackQuery(const nlohmann::json& cbq)
{
    std::string cbqId   = cbq.value("id", std::string{});
    std::string data    = cbq.value("data", std::string{});
    int64_t     chatId  = cbq.contains("message") ? cbq["message"]["chat"].value("id", (int64_t)0) : 0;
    int64_t     userId  = cbq.contains("from")    ? cbq["from"].value("id", (int64_t)0) : 0;
    std::string uname   = cbq.contains("from")    ? cbq["from"].value("username",   std::string{}) : "";
    std::string fname   = cbq.contains("from")    ? cbq["from"].value("first_name", std::string{}) : "";

    if (chatId == 0 || userId == 0) return;

    m_api.answerCallbackQuery(cbqId);

    // Admin callbacks
    if (userId == m_config->adminId() &&
        (startsWith(data, "confirm_") || startsWith(data, "reject_") || startsWith(data, "clarify_"))) {
        handleAdminCallback(chatId, cbqId, data);
        return;
    }

    UserContext& ctx = userCtx(userId);

    if (startsWith(data, "plan_")) {
        int index = std::stoi(data.substr(5));
        handlePlanSelection(chatId, userId, uname, index);
        return;
    }

    if (data == "pay_manual" || data == "pay_auto") {
        handlePaymentMethodSelection(chatId, userId, data);
        return;
    }

    if (startsWith(data, "bank_")) {
        handleAutoBankSelection(chatId, data.substr(5));
        return;
    }
}

// ---------- command / flow handlers ----------

void BotEngine::handleStart(int64_t chatId, int64_t userId,
                              const std::string& uname, const std::string& fname)
{
    userCtx(userId).state     = UserState::Idle;
    userCtx(userId).username  = uname;
    userCtx(userId).firstName = fname;

    m_api.sendMessage(chatId, m_config->welcomeMsg());
    sendMainMenu(chatId);
    log("👤 /start от " + displayName(uname, fname));
}

void BotEngine::handleTariffs(int64_t chatId)
{
    std::string text = m_config->plansMsg();
    text = replaceAll(text, "{plans}", buildPlansText());
    m_api.sendMessage(chatId, text);
}

void BotEngine::handleBuy(int64_t chatId, int64_t userId)
{
    const auto& plans = m_config->plans();
    if (plans.empty()) {
        m_api.sendMessage(chatId, "❌ Тарифы не настроены. Обратитесь к администратору.");
        return;
    }

    std::vector<BtnRow> buttons;
    for (size_t i = 0; i < plans.size(); ++i) {
        std::string label = plans[i].name + " — " + std::to_string(plans[i].price) + " ₽";
        buttons.push_back({{ label, "plan_" + std::to_string(i) }});
    }

    userCtx(userId).state = UserState::SelectingPlan;
    m_api.sendMessageWithInlineKeyboard(chatId, "Выберите тариф:", buttons);
}

void BotEngine::handleSetup(int64_t chatId)
{
    m_api.sendMessage(chatId, m_config->howToSetupMsg());
}

void BotEngine::handleSupport(int64_t chatId)
{
    std::string sup = m_config->supportUsername();
    if (sup.empty()) {
        m_api.sendMessage(chatId, "Поддержка временно недоступна.");
    } else {
        m_api.sendMessage(chatId, "📩 Служба поддержки: @" + sup);
    }
}

void BotEngine::sendMainMenu(int64_t chatId)
{
    nlohmann::json keyboard;
    nlohmann::json row1 = nlohmann::json::array();
    nlohmann::json row2 = nlohmann::json::array();
    row1.push_back({{"text", "📋 Тарифы"}});
    row1.push_back({{"text", "🛒 Купить"}});
    row2.push_back({{"text", "⚙️ Как настроить"}});
    row2.push_back({{"text", "🆘 Поддержка"}});
    nlohmann::json keyboardArr = nlohmann::json::array();
    keyboardArr.push_back(row1);
    keyboardArr.push_back(row2);
    keyboard["keyboard"]          = keyboardArr;
    keyboard["resize_keyboard"]   = true;
    keyboard["one_time_keyboard"] = false;
    m_api.sendMessage(chatId, "Главное меню:", keyboard);
}

void BotEngine::handlePlanSelection(int64_t chatId, int64_t userId,
                                     const std::string& username, int planIndex)
{
    const auto& plans = m_config->plans();
    if (planIndex < 0 || planIndex >= (int)plans.size()) return;

    const PlanConfig& plan = plans[planIndex];
    userCtx(userId).state = UserState::SelectingPaymentMethod;

    int orderId = m_orders->createOrder(userId, username, chatId, plan.name, plan.price);
    userCtx(userId).orderId = orderId;

    std::string instrText = m_config->paymentInstructionMsg();
    instrText = replaceAll(instrText, "{order_id}", std::to_string(orderId));
    instrText = replaceAll(instrText, "{plan}",     plan.name);
    instrText = replaceAll(instrText, "{price}",    std::to_string(plan.price));

    if (!m_config->manualPayLink().empty())
        instrText += "\n\n💳 Оплатить: " + m_config->manualPayLink();

    std::vector<BtnRow> buttons;
    buttons.push_back({{"💳 Ручная оплата", "pay_manual"}});
    if (m_config->showAutoPayment())
        buttons.push_back({{"🏦 Онлайн-оплата", "pay_auto"}});

    m_api.sendMessageWithInlineKeyboard(chatId, instrText, buttons);

    log("🛒 Новый заказ #" + std::to_string(orderId) + " от @" + username);
    if (onOrderUpdated) onOrderUpdated();
}

void BotEngine::handlePaymentMethodSelection(int64_t chatId, int64_t userId,
                                              const std::string& choice)
{
    if (choice == "pay_manual") {
        userCtx(userId).state = UserState::WaitingReceipt;
        m_api.sendMessage(chatId, "📎 Отправьте чек оплаты (фото или PDF).");
    } else if (choice == "pay_auto") {
        userCtx(userId).state = UserState::SelectingAutoBank;
        std::vector<BtnRow> banks;
        for (const auto& p : m_config->autoProviders()) {
            if (p.enabled)
                banks.push_back({{ p.displayName, "bank_" + p.name }});
        }
        if (banks.empty()) {
            m_api.sendMessage(chatId, "ℹ️ Онлайн-оплата временно недоступна.");
            userCtx(userId).state = UserState::Idle;
        } else {
            m_api.sendMessageWithInlineKeyboard(chatId, "Выберите банк:", banks);
        }
    }
}

void BotEngine::handleAutoBankSelection(int64_t chatId, const std::string& bank)
{
    std::string bankName = bank;
    for (const auto& p : m_config->autoProviders()) {
        if (p.name == bank) { bankName = p.displayName; break; }
    }
    m_api.sendMessage(chatId,
        "ℹ️ Оплата через " + bankName + ": функция в разработке — скоро будет доступна.");
}

void BotEngine::handleReceiptMessage(const nlohmann::json& msg,
                                      int64_t chatId, int64_t userId)
{
    bool hasPhoto = msg.contains("photo");
    bool hasDoc   = msg.contains("document");

    if (!hasPhoto && !hasDoc) {
        m_api.sendMessage(chatId, "⚠️ Чек обязателен. Пришлите фото или PDF-документ.");
        return;
    }

    std::string fileId;
    if (hasPhoto) {
        auto photos = msg["photo"];
        if (photos.is_array() && !photos.empty())
            fileId = photos.back().value("file_id", std::string{});
    } else {
        fileId = msg["document"].value("file_id", std::string{});
    }

    m_api.sendMessage(chatId, m_config->checkReceivedMsg());

    int orderId = userCtx(userId).orderId;
    if (orderId > 0) m_orders->setReceiptFileId(orderId, fileId);

    Order order = m_orders->getOrder(orderId);
    int64_t adminId = m_config->adminId();

    std::string uname     = userCtx(userId).username;
    std::string adminText = m_config->adminNotifyMsg();
    adminText = replaceAll(adminText, "{order_id}", std::to_string(orderId));
    adminText = replaceAll(adminText, "{user}",     uname.empty() ? std::to_string(userId) : "@" + uname);
    adminText = replaceAll(adminText, "{plan}",     order.planName);
    adminText = replaceAll(adminText, "{price}",    std::to_string(order.planPrice));

    std::vector<BtnRow> adminButtons = {
        {{"✅ Подтвердить", "confirm_" + std::to_string(orderId)},
         {"❌ Отклонить",   "reject_"  + std::to_string(orderId)}},
        {{"💬 Запросить уточнение", "clarify_" + std::to_string(orderId)}}
    };

    ApiResponse notifResp = m_api.sendMessageWithInlineKeyboard(adminId, adminText, adminButtons);
    if (notifResp.ok && notifResp.result.is_object()) {
        int msgId = notifResp.result.value("message_id", 0);
        if (msgId > 0) m_orders->setAdminMsgId(orderId, msgId);
    }

    // Forward the receipt to admin
    int receiptMsgId = msg.value("message_id", 0);
    m_api.forwardMessage(adminId, chatId, receiptMsgId);

    userCtx(userId).state = UserState::Idle;
    m_orders->updateOrderStatus(orderId, "pending");

    log("📎 Чек получен для заказа #" + std::to_string(orderId));
    if (onOrderUpdated) onOrderUpdated();
}

void BotEngine::handleAdminCallback(int64_t adminChatId,
                                     const std::string& callbackQueryId,
                                     const std::string& data)
{
    (void)callbackQueryId;

    auto parseId = [](const std::string& d, const std::string& prefix) -> int {
        return std::stoi(d.substr(prefix.size()));
    };

    if (startsWith(data, "confirm_")) {
        int orderId = parseId(data, "confirm_");
        m_adminWorkflow.setPendingReply(adminChatId, orderId);
        m_api.sendMessage(adminChatId,
            "📝 Заказ #" + std::to_string(orderId) +
            " — ответьте на это сообщение данными прокси, я перешлю их клиенту.");
        log("✅ Админ подтверждает заказ #" + std::to_string(orderId));

    } else if (startsWith(data, "reject_")) {
        int orderId = parseId(data, "reject_");
        m_orders->updateOrderStatus(orderId, "rejected");
        Order order = m_orders->getOrder(orderId);
        if (order.chatId > 0)
            m_api.sendMessage(order.chatId, m_config->orderRejectedMsg());
        m_api.sendMessage(adminChatId, "❌ Заказ #" + std::to_string(orderId) + " отклонён.");
        log("❌ Заказ #" + std::to_string(orderId) + " отклонён");
        if (onOrderUpdated) onOrderUpdated();

    } else if (startsWith(data, "clarify_")) {
        int orderId = parseId(data, "clarify_");
        Order order = m_orders->getOrder(orderId);
        if (order.chatId > 0)
            m_api.sendMessage(order.chatId,
                "❓ Администратор запрашивает уточнение по вашему заказу. "
                "Пожалуйста, напишите дополнительные сведения.");
        m_api.sendMessage(adminChatId,
            "💬 Запрос уточнения отправлен клиенту (заказ #" + std::to_string(orderId) + ").");
    }
}

void BotEngine::handleAdminProxyReply(int64_t adminChatId, const std::string& text)
{
    int orderId = m_adminWorkflow.getPendingOrderId(adminChatId);
    m_adminWorkflow.clearPendingReply(adminChatId);
    if (orderId < 0) return;

    m_orders->setProxyData(orderId, text);
    m_orders->updateOrderStatus(orderId, "delivered");

    Order order = m_orders->getOrder(orderId);
    if (order.chatId > 0) {
        std::string confirmMsg = m_config->orderConfirmedMsg();
        confirmMsg = replaceAll(confirmMsg, "{proxy_data}", text);
        m_api.sendMessage(order.chatId, confirmMsg);
    }

    m_api.sendMessage(adminChatId,
        "✅ Данные отправлены клиенту. Заказ #" + std::to_string(orderId) + " выполнен.");

    log("🎉 Заказ #" + std::to_string(orderId) + " выполнен, данные отправлены");
    if (onOrderUpdated) onOrderUpdated();
}

// ---------- helpers ----------

UserContext& BotEngine::userCtx(int64_t userId)
{
    return m_userCtx[userId];
}

std::string BotEngine::buildPlansText() const
{
    std::string result;
    for (const auto& p : m_config->plans())
        result += "• <b>" + p.name + "</b> — " + std::to_string(p.price) + " ₽\n";
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

std::string BotEngine::displayName(const std::string& username,
                                    const std::string& firstName) const
{
    if (!username.empty())  return "@" + username;
    if (!firstName.empty()) return firstName;
    return "Unknown";
}

void BotEngine::log(const std::string& text)
{
    if (onLog) onLog(text);
}
