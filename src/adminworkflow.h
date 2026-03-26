#pragma once

#include <unordered_map>
#include <cstdint>

class AdminWorkflow
{
public:
    void setPendingReply(int64_t adminChatId, int orderId);
    bool hasPendingReply(int64_t adminChatId) const;
    int  getPendingOrderId(int64_t adminChatId) const;
    void clearPendingReply(int64_t adminChatId);

private:
    std::unordered_map<int64_t, int> m_pending; // adminChatId -> orderId
};
