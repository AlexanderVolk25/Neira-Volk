#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <cstdint>

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

struct UserContext {
    UserState   state     = UserState::Idle;
    int         orderId   = -1;
    std::string username;
    std::string firstName;
};

class BotEngine
{
public:
    BotEngine(ConfigService* config, OrderService* orders);
    ~BotEngine();

    void start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    // Callbacks for UI notifications (called from worker thread -> UI posts message)
    std::function<void(const std::string&)> onLog;
    std::function<void()>                   onOrderUpdated;

private:
    void runLoop();
    void processUpdate(const nlohmann::json& update);
    void processMessage(const nlohmann::json& msg);
    void processCallbackQuery(const nlohmann::json& cbq);

    void handleStart(int64_t chatId, int64_t userId,
                     const std::string& username, const std::string& firstName);
    void handleTariffs(int64_t chatId);
    void handleBuy(int64_t chatId, int64_t userId);
    void handleSetup(int64_t chatId);
    void handleSupport(int64_t chatId);
    void sendMainMenu(int64_t chatId);

    void handlePlanSelection(int64_t chatId, int64_t userId,
                              const std::string& username, int planIndex);
    void handlePaymentMethodSelection(int64_t chatId, int64_t userId,
                                      const std::string& choice);
    void handleAutoBankSelection(int64_t chatId, const std::string& bank);
    void handleReceiptMessage(const nlohmann::json& msg, int64_t chatId,
                              int64_t userId);

    void handleAdminCallback(int64_t adminChatId,
                              const std::string& callbackQueryId,
                              const std::string& data);
    void handleAdminProxyReply(int64_t adminChatId, const std::string& text);

    UserContext& userCtx(int64_t userId);
    std::string  buildPlansText() const;
    std::string  displayName(const std::string& username,
                              const std::string& firstName) const;
    void         log(const std::string& text);

    ConfigService*   m_config;
    OrderService*    m_orders;
    AdminWorkflow    m_adminWorkflow;
    TelegramApiClient m_api;

    std::thread      m_thread;
    std::atomic<bool> m_running{false};
    int               m_offset = 0;

    std::unordered_map<int64_t, UserContext> m_userCtx;
};
