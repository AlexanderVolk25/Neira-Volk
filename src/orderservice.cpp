#include "orderservice.h"

#include <sqlite3.h>
#include <ctime>
#include <cstring>
#include <sstream>
#include "utils.h"

static std::string nowUtc()
{
    time_t t = time(nullptr);
    char buf[32];
    struct tm tm_info;
#ifdef _WIN32
    gmtime_s(&tm_info, &t);
#else
    gmtime_r(&t, &tm_info);
#endif
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
    return buf;
}

// Helper: column text as std::string
static std::string colText(sqlite3_stmt* s, int col)
{
    const char* p = reinterpret_cast<const char*>(sqlite3_column_text(s, col));
    return p ? p : "";
}

OrderService::OrderService() = default;

OrderService::~OrderService()
{
    if (m_db) sqlite3_close(m_db);
}

bool OrderService::init()
{
    std::string dbPath = getExeDir() + "\\bot.db";
    if (sqlite3_open(dbPath.c_str(), &m_db) != SQLITE_OK) {
        return false;
    }
    return createTables();
}

bool OrderService::createTables()
{
    const char* sql =
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
        ");"
        "CREATE TABLE IF NOT EXISTS users ("
        "  id        INTEGER PRIMARY KEY,"
        "  username  TEXT,"
        "  firstName TEXT,"
        "  lastSeen  TEXT"
        ");";
    char* errmsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errmsg);
    if (errmsg) sqlite3_free(errmsg);
    return rc == SQLITE_OK;
}

Order OrderService::orderFromStmt(sqlite3_stmt* s)
{
    Order o;
    o.id            = sqlite3_column_int(s, 0);
    o.userId        = sqlite3_column_int64(s, 1);
    o.username      = colText(s, 2);
    o.planName      = colText(s, 3);
    o.planPrice     = sqlite3_column_int(s, 4);
    o.status        = colText(s, 5);
    o.createdAt     = colText(s, 6);
    o.updatedAt     = colText(s, 7);
    o.adminMsgId    = sqlite3_column_int(s, 8);
    o.chatId        = sqlite3_column_int64(s, 9);
    o.receiptFileId = colText(s, 10);
    o.proxyData     = colText(s, 11);
    return o;
}

int OrderService::createOrder(int64_t userId, const std::string& username,
                               int64_t chatId, const std::string& planName,
                               int planPrice)
{
    std::string now = nowUtc();
    const char* sql =
        "INSERT INTO orders (userId,username,chatId,planName,planPrice,"
        "status,createdAt,updatedAt) VALUES (?,?,?,?,?,'pending',?,?)";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int64(s, 1, userId);
    sqlite3_bind_text (s, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(s, 3, chatId);
    sqlite3_bind_text (s, 4, planName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int  (s, 5, planPrice);
    sqlite3_bind_text (s, 6, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (s, 7, now.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(s);
    int id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    sqlite3_finalize(s);
    return id;
}

Order OrderService::getOrder(int orderId)
{
    const char* sql = "SELECT * FROM orders WHERE id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return {};
    sqlite3_bind_int(s, 1, orderId);
    Order o;
    if (sqlite3_step(s) == SQLITE_ROW) o = orderFromStmt(s);
    sqlite3_finalize(s);
    return o;
}

bool OrderService::updateOrderStatus(int orderId, const std::string& status)
{
    std::string now = nowUtc();
    const char* sql = "UPDATE orders SET status=?,updatedAt=? WHERE id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(s, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, now.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s, 3, orderId);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

bool OrderService::setAdminMsgId(int orderId, int msgId)
{
    const char* sql = "UPDATE orders SET adminMsgId=? WHERE id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(s, 1, msgId);
    sqlite3_bind_int(s, 2, orderId);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

bool OrderService::setReceiptFileId(int orderId, const std::string& fileId)
{
    const char* sql = "UPDATE orders SET receiptFileId=? WHERE id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(s, 1, fileId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s, 2, orderId);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

bool OrderService::setProxyData(int orderId, const std::string& proxyData)
{
    const char* sql = "UPDATE orders SET proxyData=? WHERE id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(s, 1, proxyData.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s, 2, orderId);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

std::vector<Order> OrderService::getAllOrders()
{
    const char* sql = "SELECT * FROM orders ORDER BY id DESC";
    sqlite3_stmt* s = nullptr;
    std::vector<Order> list;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(s) == SQLITE_ROW) list.push_back(orderFromStmt(s));
    sqlite3_finalize(s);
    return list;
}

std::vector<Order> OrderService::getOrdersByStatus(const std::string& status)
{
    const char* sql = "SELECT * FROM orders WHERE status=? ORDER BY id DESC";
    sqlite3_stmt* s = nullptr;
    std::vector<Order> list;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_text(s, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(s) == SQLITE_ROW) list.push_back(orderFromStmt(s));
    sqlite3_finalize(s);
    return list;
}

void OrderService::upsertUser(int64_t id, const std::string& username,
                               const std::string& firstName)
{
    std::string now = nowUtc();
    const char* sql =
        "INSERT INTO users (id,username,firstName,lastSeen) VALUES (?,?,?,?) "
        "ON CONFLICT(id) DO UPDATE SET username=excluded.username,"
        "firstName=excluded.firstName,lastSeen=excluded.lastSeen";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int64(s, 1, id);
    sqlite3_bind_text (s, 2, username.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (s, 3, firstName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (s, 4, now.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

User OrderService::getUser(int64_t id)
{
    const char* sql = "SELECT * FROM users WHERE id=?";
    sqlite3_stmt* s = nullptr;
    User u;
    if (sqlite3_prepare_v2(m_db, sql, -1, &s, nullptr) != SQLITE_OK) return u;
    sqlite3_bind_int64(s, 1, id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        u.id        = sqlite3_column_int64(s, 0);
        u.username  = colText(s, 1);
        u.firstName = colText(s, 2);
        u.lastSeen  = colText(s, 3);
    }
    sqlite3_finalize(s);
    return u;
}
