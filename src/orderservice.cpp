#include "orderservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QDebug>

OrderService::OrderService(QObject *parent)
    : QObject(parent)
{
}

bool OrderService::init()
{
    const QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("bot.db");

    m_db = QSqlDatabase::addDatabase("QSQLITE", "botdb");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "OrderService: cannot open DB:" << m_db.lastError().text();
        return false;
    }
    return createTables();
}

bool OrderService::createTables()
{
    QSqlQuery q(m_db);

    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS orders ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  userId        INTEGER NOT NULL,"
        "  username      TEXT,"
        "  planName      TEXT,"
        "  planPrice     INTEGER,"
        "  status        TEXT DEFAULT 'pending',"
        "  createdAt     TEXT,"
        "  updatedAt     TEXT,"
        "  adminMsgId    INTEGER DEFAULT 0,"
        "  chatId        INTEGER NOT NULL,"
        "  receiptFileId TEXT,"
        "  proxyData     TEXT"
        ")"
    );
    if (!ok) {
        qWarning() << "OrderService: create orders table failed:" << q.lastError().text();
        return false;
    }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id        INTEGER PRIMARY KEY,"
        "  username  TEXT,"
        "  firstName TEXT,"
        "  lastSeen  TEXT"
        ")"
    );
    if (!ok) {
        qWarning() << "OrderService: create users table failed:" << q.lastError().text();
        return false;
    }

    return true;
}

int OrderService::createOrder(qint64 userId, const QString &username,
                               qint64 chatId, const QString &planName,
                               int planPrice)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO orders (userId, username, chatId, planName, planPrice, "
              "status, createdAt, updatedAt) VALUES (?,?,?,?,?,'pending',?,?)");
    q.addBindValue(userId);
    q.addBindValue(username);
    q.addBindValue(chatId);
    q.addBindValue(planName);
    q.addBindValue(planPrice);
    q.addBindValue(now);
    q.addBindValue(now);

    if (!q.exec()) {
        qWarning() << "OrderService: createOrder failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

Order OrderService::orderFromQuery(QSqlQuery &q)
{
    Order o;
    o.id            = q.value("id").toInt();
    o.userId        = q.value("userId").toLongLong();
    o.username      = q.value("username").toString();
    o.planName      = q.value("planName").toString();
    o.planPrice     = q.value("planPrice").toInt();
    o.status        = q.value("status").toString();
    o.createdAt     = q.value("createdAt").toString();
    o.updatedAt     = q.value("updatedAt").toString();
    o.adminMsgId    = q.value("adminMsgId").toInt();
    o.chatId        = q.value("chatId").toLongLong();
    o.receiptFileId = q.value("receiptFileId").toString();
    o.proxyData     = q.value("proxyData").toString();
    return o;
}

Order OrderService::getOrder(int orderId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM orders WHERE id=?");
    q.addBindValue(orderId);
    if (q.exec() && q.next())
        return orderFromQuery(q);
    return {};
}

bool OrderService::updateOrderStatus(int orderId, const QString &status)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery q(m_db);
    q.prepare("UPDATE orders SET status=?, updatedAt=? WHERE id=?");
    q.addBindValue(status);
    q.addBindValue(now);
    q.addBindValue(orderId);
    return q.exec();
}

bool OrderService::setAdminMsgId(int orderId, int msgId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE orders SET adminMsgId=? WHERE id=?");
    q.addBindValue(msgId);
    q.addBindValue(orderId);
    return q.exec();
}

bool OrderService::setReceiptFileId(int orderId, const QString &fileId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE orders SET receiptFileId=? WHERE id=?");
    q.addBindValue(fileId);
    q.addBindValue(orderId);
    return q.exec();
}

bool OrderService::setProxyData(int orderId, const QString &proxyData)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE orders SET proxyData=? WHERE id=?");
    q.addBindValue(proxyData);
    q.addBindValue(orderId);
    return q.exec();
}

QList<Order> OrderService::getAllOrders()
{
    QSqlQuery q(m_db);
    if (!q.exec("SELECT * FROM orders ORDER BY id DESC")) {
        qWarning() << "OrderService: getAllOrders failed:" << q.lastError().text();
        return {};
    }
    QList<Order> list;
    while (q.next())
        list.append(orderFromQuery(q));
    return list;
}

QList<Order> OrderService::getOrdersByStatus(const QString &status)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM orders WHERE status=? ORDER BY id DESC");
    q.addBindValue(status);
    if (!q.exec()) {
        qWarning() << "OrderService: getOrdersByStatus failed:" << q.lastError().text();
        return {};
    }
    QList<Order> list;
    while (q.next())
        list.append(orderFromQuery(q));
    return list;
}

void OrderService::upsertUser(qint64 id, const QString &username,
                               const QString &firstName)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users (id, username, firstName, lastSeen) VALUES (?,?,?,?) "
              "ON CONFLICT(id) DO UPDATE SET username=excluded.username, "
              "firstName=excluded.firstName, lastSeen=excluded.lastSeen");
    q.addBindValue(id);
    q.addBindValue(username);
    q.addBindValue(firstName);
    q.addBindValue(now);
    if (!q.exec())
        qWarning() << "OrderService: upsertUser failed:" << q.lastError().text();
}

User OrderService::getUser(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM users WHERE id=?");
    q.addBindValue(id);
    User u;
    if (q.exec() && q.next()) {
        u.id        = q.value("id").toLongLong();
        u.username  = q.value("username").toString();
        u.firstName = q.value("firstName").toString();
        u.lastSeen  = q.value("lastSeen").toString();
    }
    return u;
}
