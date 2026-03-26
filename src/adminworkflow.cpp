#include "adminworkflow.h"

AdminWorkflow::AdminWorkflow(QObject *parent)
    : QObject(parent)
{
}

void AdminWorkflow::setPendingReply(qint64 adminChatId, int orderId)
{
    m_pending[adminChatId] = orderId;
}

bool AdminWorkflow::hasPendingReply(qint64 adminChatId) const
{
    return m_pending.contains(adminChatId);
}

int AdminWorkflow::getPendingOrderId(qint64 adminChatId) const
{
    return m_pending.value(adminChatId, -1);
}

void AdminWorkflow::clearPendingReply(qint64 adminChatId)
{
    m_pending.remove(adminChatId);
}
