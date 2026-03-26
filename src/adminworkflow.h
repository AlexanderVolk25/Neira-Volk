#pragma once

#include <QObject>
#include <QHash>

class AdminWorkflow : public QObject
{
    Q_OBJECT
public:
    explicit AdminWorkflow(QObject *parent = nullptr);

    void setPendingReply(qint64 adminChatId, int orderId);
    bool hasPendingReply(qint64 adminChatId) const;
    int  getPendingOrderId(qint64 adminChatId) const;
    void clearPendingReply(qint64 adminChatId);

private:
    QHash<qint64, int> m_pending; // adminChatId -> orderId
};
