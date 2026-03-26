#include "configservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ConfigService::ConfigService(QObject *parent)
    : QObject(parent)
{
    setDefaults();
}

void ConfigService::setDefaults()
{
    m_plans = {
        {"1 день",    100, 1, 0, 0},
        {"1 месяц",   200, 0, 1, 0},
        {"3 месяца",  400, 0, 3, 0},
        {"1 год",     600, 0, 0, 1}
    };

    m_autoProviders = {
        {"tbank",  false, "Т-Банк", "", "", "", "Оплата через Т-Банк API"},
        {"sber",   false, "Сбер",   "", "", "", "Оплата через Сбер API"},
        {"other",  false, "Другой", "", "", "", "Другой провайдер"}
    };

    m_welcomeMsg          = "Добро пожаловать! Я помогу вам оформить подписку.\n\nВыберите действие:";
    m_plansMsg            = "📋 Доступные тарифы:\n\n{plans}";
    m_howToSetupMsg       = "⚙️ Инструкция по настройке:\n\n1. Приобретите подписку\n2. После подтверждения вы получите данные\n3. Настройте приложение согласно инструкции";
    m_paymentInstructionMsg = "🧾 Заказ #{order_id}\nТариф: {plan}\nСумма: {price} ₽\n\nОткройте ссылку и оплатите ровно {price} ₽\nПосле оплаты отправьте чек (фото или PDF)";
    m_adminNotifyMsg      = "📦 Новый заказ #{order_id}\nПользователь: {user}\nТариф: {plan}\nСумма: {price} ₽";
    m_checkReceivedMsg    = "Чек получен ✅ Ожидайте подтверждения";
    m_orderConfirmedMsg   = "✅ Ваш заказ подтверждён!\n\nДанные для доступа:\n{proxy_data}";
    m_orderRejectedMsg    = "❌ Ваш заказ отклонён.\n\nЕсли у вас есть вопросы, обратитесь в поддержку.";
    m_channelPostText     = "📢 Новое объявление";
}

static QString configPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("config.json");
}

void ConfigService::load()
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        // No file yet — use defaults
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject root = doc.object();

    m_botToken        = root.value("botToken").toString(m_botToken);
    m_adminId         = root.value("adminId").toVariant().toLongLong();
    m_supportUsername = root.value("supportUsername").toString(m_supportUsername);
    m_channelUsername = root.value("channelUsername").toString(m_channelUsername);
    m_showAutoPayment = root.value("showAutoPayment").toBool(m_showAutoPayment);
    m_manualPayLink   = root.value("manualPayLink").toString(m_manualPayLink);
    m_channelPostText = root.value("channelPostText").toString(m_channelPostText);

    // Plans
    if (root.contains("plans") && root["plans"].isArray()) {
        QJsonArray arr = root["plans"].toArray();
        QList<PlanConfig> plans;
        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            PlanConfig p;
            p.name   = o.value("name").toString();
            p.price  = o.value("price").toInt();
            p.days   = o.value("days").toInt();
            p.months = o.value("months").toInt();
            p.years  = o.value("years").toInt();
            plans.append(p);
        }
        if (!plans.isEmpty())
            m_plans = plans;
    }

    // Auto providers
    if (root.contains("autoProviders") && root["autoProviders"].isArray()) {
        QJsonArray arr = root["autoProviders"].toArray();
        QList<AutoProviderConfig> providers;
        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            AutoProviderConfig p;
            p.name        = o.value("name").toString();
            p.enabled     = o.value("enabled").toBool();
            p.displayName = o.value("displayName").toString();
            p.apiKey      = o.value("apiKey").toString();
            p.terminalKey = o.value("terminalKey").toString();
            p.password    = o.value("password").toString();
            p.description = o.value("description").toString();
            providers.append(p);
        }
        if (!providers.isEmpty())
            m_autoProviders = providers;
    }

    // Message templates
    auto readMsg = [&](const QString &key, const QString &def) -> QString {
        return root.contains(key) ? root.value(key).toString() : def;
    };

    m_welcomeMsg            = readMsg("welcomeMsg",            m_welcomeMsg);
    m_plansMsg              = readMsg("plansMsg",              m_plansMsg);
    m_howToSetupMsg         = readMsg("howToSetupMsg",         m_howToSetupMsg);
    m_paymentInstructionMsg = readMsg("paymentInstructionMsg", m_paymentInstructionMsg);
    m_adminNotifyMsg        = readMsg("adminNotifyMsg",        m_adminNotifyMsg);
    m_checkReceivedMsg      = readMsg("checkReceivedMsg",      m_checkReceivedMsg);
    m_orderConfirmedMsg     = readMsg("orderConfirmedMsg",     m_orderConfirmedMsg);
    m_orderRejectedMsg      = readMsg("orderRejectedMsg",      m_orderRejectedMsg);
}

void ConfigService::save()
{
    QJsonObject root;
    root["botToken"]        = m_botToken;
    root["adminId"]         = m_adminId;
    root["supportUsername"] = m_supportUsername;
    root["channelUsername"] = m_channelUsername;
    root["showAutoPayment"] = m_showAutoPayment;
    root["manualPayLink"]   = m_manualPayLink;
    root["channelPostText"] = m_channelPostText;

    // Plans
    QJsonArray plansArr;
    for (const PlanConfig &p : m_plans) {
        QJsonObject o;
        o["name"]   = p.name;
        o["price"]  = p.price;
        o["days"]   = p.days;
        o["months"] = p.months;
        o["years"]  = p.years;
        plansArr.append(o);
    }
    root["plans"] = plansArr;

    // Auto providers
    QJsonArray provArr;
    for (const AutoProviderConfig &p : m_autoProviders) {
        QJsonObject o;
        o["name"]        = p.name;
        o["enabled"]     = p.enabled;
        o["displayName"] = p.displayName;
        o["apiKey"]      = p.apiKey;
        o["terminalKey"] = p.terminalKey;
        o["password"]    = p.password;
        o["description"] = p.description;
        provArr.append(o);
    }
    root["autoProviders"] = provArr;

    // Message templates
    root["welcomeMsg"]            = m_welcomeMsg;
    root["plansMsg"]              = m_plansMsg;
    root["howToSetupMsg"]         = m_howToSetupMsg;
    root["paymentInstructionMsg"] = m_paymentInstructionMsg;
    root["adminNotifyMsg"]        = m_adminNotifyMsg;
    root["checkReceivedMsg"]      = m_checkReceivedMsg;
    root["orderConfirmedMsg"]     = m_orderConfirmedMsg;
    root["orderRejectedMsg"]      = m_orderRejectedMsg;

    QFile file(configPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}
