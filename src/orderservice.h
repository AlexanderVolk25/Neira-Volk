#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

struct Order {
    int     id           = 0;
    qint64  userId       = 0;
    QString username;
    QString planName;
    int     planPrice    = 0;
    QString status;       // pending / confirmed / rejected / delivered
    QString createdAt;
    QString updatedAt;
    int     adminMsgId   = 0;
    qint64  chatId       = 0;
    QString receiptFileId;
    QString proxyData;
};

struct User {
    qint64  id        = 0;
    QString username;
    QString firstName;
    QString lastSeen;
};

class OrderService : public QObject
{
    Q_OBJECT
public:
    explicit OrderService(QObject *parent = nullptr);

    bool init();

    int     createOrder(qint64 userId, const QString &username, qint64 chatId,
                        const QString &planName, int planPrice);
    Order   getOrder(int orderId);
    bool    updateOrderStatus(int orderId, const QString &status);
    bool    setAdminMsgId(int orderId, int msgId);
    bool    setReceiptFileId(int orderId, const QString &fileId);
    bool    setProxyData(int orderId, const QString &proxyData);
    QList<Order> getAllOrders();
    QList<Order> getOrdersByStatus(const QString &status);

    void    upsertUser(qint64 id, const QString &username, const QString &firstName);
    User    getUser(qint64 id);

private:
    bool createTables();
    Order orderFromQuery(QSqlQuery &q);

    QSqlDatabase m_db;
};
