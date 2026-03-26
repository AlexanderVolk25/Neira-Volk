#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

struct PlanConfig {
    QString name;
    int price = 0;
    int days = 0;
    int months = 0;
    int years = 0;
};

struct AutoProviderConfig {
    QString name;
    bool enabled = false;
    QString displayName;
    QString apiKey;
    QString terminalKey;
    QString password;
    QString description;
};

class ConfigService : public QObject
{
    Q_OBJECT
public:
    explicit ConfigService(QObject *parent = nullptr);

    void load();
    void save();

    // Bot settings
    QString botToken() const { return m_botToken; }
    void setBotToken(const QString &v) { m_botToken = v; }

    qint64 adminId() const { return m_adminId; }
    void setAdminId(qint64 v) { m_adminId = v; }

    QString supportUsername() const { return m_supportUsername; }
    void setSupportUsername(const QString &v) { m_supportUsername = v; }

    QString channelUsername() const { return m_channelUsername; }
    void setChannelUsername(const QString &v) { m_channelUsername = v; }

    bool showAutoPayment() const { return m_showAutoPayment; }
    void setShowAutoPayment(bool v) { m_showAutoPayment = v; }

    // Plans
    QList<PlanConfig> plans() const { return m_plans; }
    void setPlans(const QList<PlanConfig> &plans) { m_plans = plans; }

    // Manual payment
    QString manualPayLink() const { return m_manualPayLink; }
    void setManualPayLink(const QString &v) { m_manualPayLink = v; }

    // Auto providers
    QList<AutoProviderConfig> autoProviders() const { return m_autoProviders; }
    void setAutoProviders(const QList<AutoProviderConfig> &providers) { m_autoProviders = providers; }

    // Message templates
    QString welcomeMsg() const { return m_welcomeMsg; }
    void setWelcomeMsg(const QString &v) { m_welcomeMsg = v; }

    QString plansMsg() const { return m_plansMsg; }
    void setPlansMsg(const QString &v) { m_plansMsg = v; }

    QString howToSetupMsg() const { return m_howToSetupMsg; }
    void setHowToSetupMsg(const QString &v) { m_howToSetupMsg = v; }

    QString paymentInstructionMsg() const { return m_paymentInstructionMsg; }
    void setPaymentInstructionMsg(const QString &v) { m_paymentInstructionMsg = v; }

    QString adminNotifyMsg() const { return m_adminNotifyMsg; }
    void setAdminNotifyMsg(const QString &v) { m_adminNotifyMsg = v; }

    QString checkReceivedMsg() const { return m_checkReceivedMsg; }
    void setCheckReceivedMsg(const QString &v) { m_checkReceivedMsg = v; }

    QString orderConfirmedMsg() const { return m_orderConfirmedMsg; }
    void setOrderConfirmedMsg(const QString &v) { m_orderConfirmedMsg = v; }

    QString orderRejectedMsg() const { return m_orderRejectedMsg; }
    void setOrderRejectedMsg(const QString &v) { m_orderRejectedMsg = v; }

    QString channelPostText() const { return m_channelPostText; }
    void setChannelPostText(const QString &v) { m_channelPostText = v; }

private:
    void setDefaults();

    QString m_botToken;
    qint64 m_adminId = 0;
    QString m_supportUsername;
    QString m_channelUsername;
    bool m_showAutoPayment = false;

    QList<PlanConfig> m_plans;
    QString m_manualPayLink;
    QList<AutoProviderConfig> m_autoProviders;

    QString m_welcomeMsg;
    QString m_plansMsg;
    QString m_howToSetupMsg;
    QString m_paymentInstructionMsg;
    QString m_adminNotifyMsg;
    QString m_checkReceivedMsg;
    QString m_orderConfirmedMsg;
    QString m_orderRejectedMsg;
    QString m_channelPostText;
};
