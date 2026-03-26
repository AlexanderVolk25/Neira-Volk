#include "configservice.h"

#include <fstream>
#include <sstream>
#include "utils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ConfigService::ConfigService()
{
    setDefaults();
}

void ConfigService::setDefaults()
{
    m_plans = {
        {"1 день",   100, 1, 0, 0},
        {"1 месяц",  200, 0, 1, 0},
        {"3 месяца", 400, 0, 3, 0},
        {"1 год",    600, 0, 0, 1}
    };

    m_autoProviders = {
        {"tbank", false, "Т-Банк",  "", "", "", "Оплата через Т-Банк API"},
        {"sber",  false, "Сбер",    "", "", "", "Оплата через Сбер API"},
        {"other", false, "Другой",  "", "", "", "Другой провайдер"}
    };

    m_welcomeMsg            = "Добро пожаловать! Я помогу вам оформить подписку.\n\nВыберите действие:";
    m_plansMsg              = "📋 Доступные тарифы:\n\n{plans}";
    m_howToSetupMsg         = "⚙️ Инструкция по настройке:\n\n1. Приобретите подписку\n2. После подтверждения вы получите данные\n3. Настройте приложение согласно инструкции";
    m_paymentInstructionMsg = "🧾 Заказ #{order_id}\nТариф: {plan}\nСумма: {price} ₽\n\nОткройте ссылку и оплатите ровно {price} ₽\nПосле оплаты отправьте чек (фото или PDF)";
    m_adminNotifyMsg        = "📦 Новый заказ #{order_id}\nПользователь: {user}\nТариф: {plan}\nСумма: {price} ₽";
    m_checkReceivedMsg      = "Чек получен ✅ Ожидайте подтверждения";
    m_orderConfirmedMsg     = "✅ Ваш заказ подтверждён!\n\nДанные для доступа:\n{proxy_data}";
    m_orderRejectedMsg      = "❌ Ваш заказ отклонён.\n\nЕсли у вас есть вопросы, обратитесь в поддержку.";
    m_channelPostText       = "📢 Новое объявление";
}

static std::string configPath()
{
    return getExeDir() + "\\config.json";
}

void ConfigService::load()
{
    std::ifstream file(configPath());
    if (!file.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    json root;
    try {
        root = json::parse(content);
    } catch (...) {
        return;
    }

    if (!root.is_object()) return;

    auto str  = [&](const char* k, const std::string& def) -> std::string {
        return root.contains(k) && root[k].is_string() ? root[k].get<std::string>() : def;
    };
    auto bval = [&](const char* k, bool def) -> bool {
        return root.contains(k) && root[k].is_boolean() ? root[k].get<bool>() : def;
    };

    m_botToken        = str("botToken",        m_botToken);
    m_adminId         = root.contains("adminId") ? root["adminId"].get<int64_t>() : m_adminId;
    m_supportUsername = str("supportUsername", m_supportUsername);
    m_channelUsername = str("channelUsername", m_channelUsername);
    m_showAutoPayment = bval("showAutoPayment", m_showAutoPayment);
    m_manualPayLink   = str("manualPayLink",   m_manualPayLink);
    m_channelPostText = str("channelPostText", m_channelPostText);

    if (root.contains("plans") && root["plans"].is_array()) {
        std::vector<PlanConfig> plans;
        for (const auto& o : root["plans"]) {
            PlanConfig p;
            p.name   = o.value("name",   std::string{});
            p.price  = o.value("price",  0);
            p.days   = o.value("days",   0);
            p.months = o.value("months", 0);
            p.years  = o.value("years",  0);
            plans.push_back(p);
        }
        if (!plans.empty()) m_plans = plans;
    }

    if (root.contains("autoProviders") && root["autoProviders"].is_array()) {
        std::vector<AutoProviderConfig> providers;
        for (const auto& o : root["autoProviders"]) {
            AutoProviderConfig p;
            p.name        = o.value("name",        std::string{});
            p.enabled     = o.value("enabled",     false);
            p.displayName = o.value("displayName", std::string{});
            p.apiKey      = o.value("apiKey",      std::string{});
            p.terminalKey = o.value("terminalKey", std::string{});
            p.password    = o.value("password",    std::string{});
            p.description = o.value("description", std::string{});
            providers.push_back(p);
        }
        if (!providers.empty()) m_autoProviders = providers;
    }

    m_welcomeMsg            = str("welcomeMsg",            m_welcomeMsg);
    m_plansMsg              = str("plansMsg",              m_plansMsg);
    m_howToSetupMsg         = str("howToSetupMsg",         m_howToSetupMsg);
    m_paymentInstructionMsg = str("paymentInstructionMsg", m_paymentInstructionMsg);
    m_adminNotifyMsg        = str("adminNotifyMsg",        m_adminNotifyMsg);
    m_checkReceivedMsg      = str("checkReceivedMsg",      m_checkReceivedMsg);
    m_orderConfirmedMsg     = str("orderConfirmedMsg",     m_orderConfirmedMsg);
    m_orderRejectedMsg      = str("orderRejectedMsg",      m_orderRejectedMsg);
}

void ConfigService::save()
{
    json root;
    root["botToken"]        = m_botToken;
    root["adminId"]         = m_adminId;
    root["supportUsername"] = m_supportUsername;
    root["channelUsername"] = m_channelUsername;
    root["showAutoPayment"] = m_showAutoPayment;
    root["manualPayLink"]   = m_manualPayLink;
    root["channelPostText"] = m_channelPostText;

    json plansArr = json::array();
    for (const auto& p : m_plans) {
        json o;
        o["name"]   = p.name;
        o["price"]  = p.price;
        o["days"]   = p.days;
        o["months"] = p.months;
        o["years"]  = p.years;
        plansArr.push_back(o);
    }
    root["plans"] = plansArr;

    json provArr = json::array();
    for (const auto& p : m_autoProviders) {
        json o;
        o["name"]        = p.name;
        o["enabled"]     = p.enabled;
        o["displayName"] = p.displayName;
        o["apiKey"]      = p.apiKey;
        o["terminalKey"] = p.terminalKey;
        o["password"]    = p.password;
        o["description"] = p.description;
        provArr.push_back(o);
    }
    root["autoProviders"] = provArr;

    root["welcomeMsg"]            = m_welcomeMsg;
    root["plansMsg"]              = m_plansMsg;
    root["howToSetupMsg"]         = m_howToSetupMsg;
    root["paymentInstructionMsg"] = m_paymentInstructionMsg;
    root["adminNotifyMsg"]        = m_adminNotifyMsg;
    root["checkReceivedMsg"]      = m_checkReceivedMsg;
    root["orderConfirmedMsg"]     = m_orderConfirmedMsg;
    root["orderRejectedMsg"]      = m_orderRejectedMsg;

    std::ofstream file(configPath());
    if (file.is_open()) {
        file << root.dump(4);
        file.close();
    }
}
