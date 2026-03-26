#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct Order {
    int         id            = 0;
    int64_t     userId        = 0;
    std::string username;
    std::string planName;
    int         planPrice     = 0;
    std::string status;       // pending / confirmed / rejected / delivered
    std::string createdAt;
    std::string updatedAt;
    int         adminMsgId    = 0;
    int64_t     chatId        = 0;
    std::string receiptFileId;
    std::string proxyData;
};

struct User {
    int64_t     id        = 0;
    std::string username;
    std::string firstName;
    std::string lastSeen;
};

// Forward declarations to avoid including sqlite3.h in headers
struct sqlite3;
struct sqlite3_stmt;

class OrderService
{
public:
    OrderService();
    ~OrderService();

    bool init();

    int              createOrder(int64_t userId, const std::string& username,
                                 int64_t chatId, const std::string& planName,
                                 int planPrice);
    Order            getOrder(int orderId);
    bool             updateOrderStatus(int orderId, const std::string& status);
    bool             setAdminMsgId(int orderId, int msgId);
    bool             setReceiptFileId(int orderId, const std::string& fileId);
    bool             setProxyData(int orderId, const std::string& proxyData);
    std::vector<Order> getAllOrders();
    std::vector<Order> getOrdersByStatus(const std::string& status);

    void             upsertUser(int64_t id, const std::string& username,
                                const std::string& firstName);
    User             getUser(int64_t id);

private:
    bool  createTables();
    Order orderFromStmt(sqlite3_stmt* stmt);

    sqlite3* m_db = nullptr;
};
