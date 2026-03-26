#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct PlanConfig {
    std::string name;
    int price  = 0;
    int days   = 0;
    int months = 0;
    int years  = 0;
};

struct AutoProviderConfig {
    std::string name;
    bool        enabled     = false;
    std::string displayName;
    std::string apiKey;
    std::string terminalKey;
    std::string password;
    std::string description;
};

class ConfigService
{
public:
    ConfigService();

    void load();
    void save();

    // Bot settings
    const std::string& botToken() const { return m_botToken; }
    void setBotToken(const std::string& v) { m_botToken = v; }

    int64_t adminId() const { return m_adminId; }
    void setAdminId(int64_t v) { m_adminId = v; }

    const std::string& supportUsername() const { return m_supportUsername; }
    void setSupportUsername(const std::string& v) { m_supportUsername = v; }

    const std::string& channelUsername() const { return m_channelUsername; }
    void setChannelUsername(const std::string& v) { m_channelUsername = v; }

    bool showAutoPayment() const { return m_showAutoPayment; }
    void setShowAutoPayment(bool v) { m_showAutoPayment = v; }

    // Plans
    const std::vector<PlanConfig>& plans() const { return m_plans; }
    void setPlans(const std::vector<PlanConfig>& plans) { m_plans = plans; }

    // Manual payment
    const std::string& manualPayLink() const { return m_manualPayLink; }
    void setManualPayLink(const std::string& v) { m_manualPayLink = v; }

    // Auto providers
    const std::vector<AutoProviderConfig>& autoProviders() const { return m_autoProviders; }
    void setAutoProviders(const std::vector<AutoProviderConfig>& providers) { m_autoProviders = providers; }

    // Message templates
    const std::string& welcomeMsg() const { return m_welcomeMsg; }
    void setWelcomeMsg(const std::string& v) { m_welcomeMsg = v; }

    const std::string& plansMsg() const { return m_plansMsg; }
    void setPlansMsg(const std::string& v) { m_plansMsg = v; }

    const std::string& howToSetupMsg() const { return m_howToSetupMsg; }
    void setHowToSetupMsg(const std::string& v) { m_howToSetupMsg = v; }

    const std::string& paymentInstructionMsg() const { return m_paymentInstructionMsg; }
    void setPaymentInstructionMsg(const std::string& v) { m_paymentInstructionMsg = v; }

    const std::string& adminNotifyMsg() const { return m_adminNotifyMsg; }
    void setAdminNotifyMsg(const std::string& v) { m_adminNotifyMsg = v; }

    const std::string& checkReceivedMsg() const { return m_checkReceivedMsg; }
    void setCheckReceivedMsg(const std::string& v) { m_checkReceivedMsg = v; }

    const std::string& orderConfirmedMsg() const { return m_orderConfirmedMsg; }
    void setOrderConfirmedMsg(const std::string& v) { m_orderConfirmedMsg = v; }

    const std::string& orderRejectedMsg() const { return m_orderRejectedMsg; }
    void setOrderRejectedMsg(const std::string& v) { m_orderRejectedMsg = v; }

    const std::string& channelPostText() const { return m_channelPostText; }
    void setChannelPostText(const std::string& v) { m_channelPostText = v; }

private:
    void setDefaults();

    std::string m_botToken;
    int64_t     m_adminId = 0;
    std::string m_supportUsername;
    std::string m_channelUsername;
    bool        m_showAutoPayment = false;

    std::vector<PlanConfig>        m_plans;
    std::string                    m_manualPayLink;
    std::vector<AutoProviderConfig> m_autoProviders;

    std::string m_welcomeMsg;
    std::string m_plansMsg;
    std::string m_howToSetupMsg;
    std::string m_paymentInstructionMsg;
    std::string m_adminNotifyMsg;
    std::string m_checkReceivedMsg;
    std::string m_orderConfirmedMsg;
    std::string m_orderRejectedMsg;
    std::string m_channelPostText;
};
