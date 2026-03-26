#include "adminworkflow.h"

void AdminWorkflow::setPendingReply(int64_t adminChatId, int orderId)
{
    m_pending[adminChatId] = orderId;
}

bool AdminWorkflow::hasPendingReply(int64_t adminChatId) const
{
    return m_pending.count(adminChatId) > 0;
}

int AdminWorkflow::getPendingOrderId(int64_t adminChatId) const
{
    auto it = m_pending.find(adminChatId);
    return it != m_pending.end() ? it->second : -1;
}

void AdminWorkflow::clearPendingReply(int64_t adminChatId)
{
    m_pending.erase(adminChatId);
}
