#include "mysql_connector.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/util.h"


namespace sylar {
namespace db {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool MySQLConf::loadFromYaml(const YAML::Node& node) {
    try {
        host = node["host"].as<std::string>();
        port = node["port"].as<uint32_t>();
        user = node["user"].as<std::string>();
        password = node["password"].as<std::string>();
        database = node["database"].as<std::string>();
        
        if(node["charset"]) {
            charset = node["charset"].as<std::string>();
        }
        if(node["timeout"]) {
            timeout = node["timeout"].as<uint32_t>();
        }
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load MySQL config error: " << e.what();
        return false;
    }
}

// ==================== MySQLRow 实现 ====================

MySQLRow::MySQLRow(sql::ResultSet* resultSet) 
    : m_resultSet(resultSet) {
    if (m_resultSet) {
        m_metaData = m_resultSet->getMetaData();
    } else {
        m_metaData = nullptr;
    }
}

std::string MySQLRow::getString(const std::string& column) const {
    if (!m_resultSet) return "";
    try {
        return m_resultSet->getString(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getString error: " << e.what();
        return "";
    }
}

std::string MySQLRow::getString(int columnIndex) const {
    if (!m_resultSet) return "";
    try {
        return m_resultSet->getString(columnIndex + 1); // JDBC索引从1开始
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getString error: " << e.what();
        return "";
    }
}

int32_t MySQLRow::getInt32(const std::string& column) const {
    if (!m_resultSet) return 0;
    try {
        return m_resultSet->getInt(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getInt32 error: " << e.what();
        return 0;
    }
}

int32_t MySQLRow::getInt32(int columnIndex) const {
    if (!m_resultSet) return 0;
    try {
        return m_resultSet->getInt(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getInt32 error: " << e.what();
        return 0;
    }
}

int64_t MySQLRow::getInt64(const std::string& column) const {
    if (!m_resultSet) return 0;
    try {
        return m_resultSet->getInt64(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getInt64 error: " << e.what();
        return 0;
    }
}

int64_t MySQLRow::getInt64(int columnIndex) const {
    if (!m_resultSet) return 0;
    try {
        return m_resultSet->getInt64(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getInt64 error: " << e.what();
        return 0;
    }
}

double MySQLRow::getDouble(const std::string& column) const {
    if (!m_resultSet) return 0.0;
    try {
        return m_resultSet->getDouble(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getDouble error: " << e.what();
        return 0.0;
    }
}

double MySQLRow::getDouble(int columnIndex) const {
    if (!m_resultSet) return 0.0;
    try {
        return m_resultSet->getDouble(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getDouble error: " << e.what();
        return 0.0;
    }
}

bool MySQLRow::getBoolean(const std::string& column) const {
    if (!m_resultSet) return false;
    try {
        return m_resultSet->getBoolean(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getBoolean error: " << e.what();
        return false;
    }
}

bool MySQLRow::getBoolean(int columnIndex) const {
    if (!m_resultSet) return false;
    try {
        return m_resultSet->getBoolean(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getBoolean error: " << e.what();
        return false;
    }
}

bool MySQLRow::isNull(const std::string& column) const {
    if (!m_resultSet) return true;
    try {
        return m_resultSet->isNull(column);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL isNull error: " << e.what();
        return true;
    }
}

bool MySQLRow::isNull(int columnIndex) const {
    if (!m_resultSet) return true;
    try {
        return m_resultSet->isNull(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL isNull error: " << e.what();
        return true;
    }
}

std::string MySQLRow::getColumnName(uint64_t columnIndex) const {
    if (!m_metaData || columnIndex >= getColumnCount()) return "";
    try {
        return m_metaData->getColumnName(columnIndex + 1);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getColumnName error: " << e.what();
        return "";
    }
}

// ==================== MySQLResult 实现 ====================

MySQLResult::MySQLResult(sql::ResultSet* resultSet) 
    : m_resultSet(resultSet), m_ownsResultSet(true) {
}

MySQLResult::~MySQLResult() {
    if (m_ownsResultSet && m_resultSet) {
        delete m_resultSet;
    }
}

bool MySQLResult::next() {
    if (!m_resultSet) return false;
    try {
        return m_resultSet->next();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL result next error: " << e.what();
        return false;
    }
}

MySQLRow MySQLResult::getRow() const {
    return MySQLRow(m_resultSet);
}

uint64_t MySQLResult::getRowCount() const {
    if (!m_resultSet) return 0;
    try {
        return m_resultSet->rowsCount();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getRowCount error: " << e.what();
        return 0;
    }
}

uint64_t MySQLResult::getColumnCount() const {
    if (!m_resultSet) return 0;
    try {
        sql::ResultSetMetaData* meta = m_resultSet->getMetaData();
        return meta ? meta->getColumnCount() : 0;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getColumnCount error: " << e.what();
        return 0;
    }
}

std::vector<std::string> MySQLResult::getColumnNames() const {
    std::vector<std::string> names;
    if (!m_resultSet) return names;
    
    try {
        sql::ResultSetMetaData* meta = m_resultSet->getMetaData();
        if (meta) {
            uint64_t count = meta->getColumnCount();
            for (uint64_t i = 1; i <= count; ++i) {
                names.push_back(meta->getColumnName(i));
            }
        }
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getColumnNames error: " << e.what();
    }
    
    return names;
}

bool MySQLResult::isEmpty() const {
    return getRowCount() == 0;
}

// ==================== MySQLPreparedStatement 实现 ====================

MySQLPreparedStatement::MySQLPreparedStatement(sql::PreparedStatement* stmt) 
    : m_stmt(stmt) {
}

MySQLPreparedStatement::~MySQLPreparedStatement() {
    if (m_stmt) {
        delete m_stmt;
    }
}

void MySQLPreparedStatement::setNull(int parameterIndex) {
    if (!m_stmt) return;
    try {
        m_stmt->setNull(parameterIndex, sql::DataType::SQLNULL);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setNull error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setBoolean(int parameterIndex, bool value) {
    if (!m_stmt) return;
    try {
        m_stmt->setBoolean(parameterIndex, value);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setBoolean error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setInt32(int parameterIndex, int32_t value) {
    if (!m_stmt) return;
    try {
        m_stmt->setInt(parameterIndex, value);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setInt32 error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setInt64(int parameterIndex, int64_t value) {
    if (!m_stmt) return;
    try {
        m_stmt->setInt64(parameterIndex, value);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setInt64 error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setDouble(int parameterIndex, double value) {
    if (!m_stmt) return;
    try {
        m_stmt->setDouble(parameterIndex, value);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setDouble error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setString(int parameterIndex, const std::string& value) {
    if (!m_stmt) return;
    try {
        m_stmt->setString(parameterIndex, value);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setString error: " + std::string(e.what()));
    }
}

void MySQLPreparedStatement::setDateTime(int parameterIndex, const std::string& datetime) {
    if (!m_stmt) return;
    try {
        m_stmt->setDateTime(parameterIndex, datetime);
    } catch (const sql::SQLException& e) {
        throw MySQLException("setDateTime error: " + std::string(e.what()));
    }
}

bool MySQLPreparedStatement::execute() {
    if (!m_stmt) return false;
    try {
        m_stmt->execute();
        return true;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL prepared statement execute error: " << e.what();
        return false;
    }
}

MySQLResult::ptr MySQLPreparedStatement::executeQuery() {
    if (!m_stmt) return nullptr;
    try {
        sql::ResultSet* result = m_stmt->executeQuery();
        return result ? std::make_shared<MySQLResult>(result) : nullptr;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL prepared statement executeQuery error: " << e.what();
        return nullptr;
    }
}

int64_t MySQLPreparedStatement::executeUpdate() {
    if (!m_stmt) return 0;
    try {
        return m_stmt->executeUpdate();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL prepared statement executeUpdate error: " << e.what();
        return -1;
    }
}

uint64_t MySQLPreparedStatement::getUpdateCount() const {
    if (!m_stmt) return 0;
    try {
        return m_stmt->getUpdateCount();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getUpdateCount error: " << e.what();
        return -1;
    }
}

// ==================== MySQLConnection 实现 ====================

MySQLConnection::MySQLConnection() 
    : m_connection(nullptr), m_driver(nullptr) {
}

MySQLConnection::~MySQLConnection() {
    close();
}

bool MySQLConnection::connect(const std::string& host, uint16_t port,
                            const std::string& username, const std::string& password,
                            const std::string& database,
                            const std::map<std::string, std::string>& properties) {
    try {
        m_driver = sql::mysql::get_mysql_driver_instance();
        if (!m_driver) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to get MySQL driver instance";
            return false;
        }
        
        std::string url = "tcp://" + host + ":" + std::to_string(port);
        m_connection = m_driver->connect(url, username, password);
        if (!m_connection) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to connect to MySQL: " << url;
            return false;
        }
        
        m_connection->setSchema(database);
        
        // 设置连接属性
        for (const auto& prop : properties) {
            m_connection->setClientOption(prop.first, prop.second);
        }
        
        m_host = host;
        m_port = port;
        m_username = username;
        m_password = password;
        m_database = database;
        
        SYLAR_LOG_INFO(g_logger) << "Connected to MySQL: " << host << ":" << port << "/" << database;
        return true;
        
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL connection error: " << e.what();
        return false;
    }
}

void MySQLConnection::close() {
    if (m_connection) {
        try {
            m_connection->close();
            delete m_connection;
            m_connection = nullptr;
        } catch (const sql::SQLException& e) {
            SYLAR_LOG_ERROR(g_logger) << "MySQL close error: " << e.what();
        }
    }
}

bool MySQLConnection::isClosed() const {
    if (!m_connection) return true;
    try {
        return m_connection->isClosed();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL isClosed error: " << e.what();
        return true;
    }
}

bool MySQLConnection::reconnect() {
    close();
    return connect(m_host, m_port, m_username, m_password, m_database);
}

bool MySQLConnection::execute(const std::string& sql) {
    if (!m_connection || isClosed()) return false;
    try {
        sql::Statement* stmt = m_connection->createStatement();
        bool result = stmt->execute(sql);
        delete stmt;
        return result;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL execute error: " << e.what() << " SQL: " << sql;
        return false;
    }
}

MySQLResult::ptr MySQLConnection::executeQuery(const std::string& sql) {
    if (!m_connection || isClosed()) return nullptr;
    try {
        sql::Statement* stmt = m_connection->createStatement();
        sql::ResultSet* result = stmt->executeQuery(sql);
        MySQLResult::ptr ret = result ? std::make_shared<MySQLResult>(result) : nullptr;
        delete stmt;
        return ret;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL executeQuery error: " << e.what() << " SQL: " << sql;
        return nullptr;
    }
}

int64_t MySQLConnection::executeUpdate(const std::string& sql) {
    if (!m_connection || isClosed()) return 0;
    try {
        sql::Statement* stmt = m_connection->createStatement();
        int64_t result = stmt->executeUpdate(sql);
        delete stmt;
        return result;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL executeUpdate error: " << e.what() << " SQL: " << sql;
        return 0;
    }
}

MySQLPreparedStatement::ptr MySQLConnection::prepareStatement(const std::string& sql) {
    if (!m_connection || isClosed()) return nullptr;
    try {
        sql::PreparedStatement* stmt = m_connection->prepareStatement(sql);
        return stmt ? std::make_shared<MySQLPreparedStatement>(stmt) : nullptr;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL prepareStatement error: " << e.what() << " SQL: " << sql;
        return nullptr;
    }
}

void MySQLConnection::setAutoCommit(bool autoCommit) {
    if (!m_connection) return;
    try {
        m_connection->setAutoCommit(autoCommit);
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL setAutoCommit error: " << e.what();
    }
}

bool MySQLConnection::getAutoCommit() const {
    if (!m_connection) return true;
    try {
        return m_connection->getAutoCommit();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getAutoCommit error: " << e.what();
        return true;
    }
}

void MySQLConnection::commit() {
    if (!m_connection) return;
    try {
        m_connection->commit();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL commit error: " << e.what();
    }
}

void MySQLConnection::rollback() {
    if (!m_connection) return;
    try {
        m_connection->rollback();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL rollback error: " << e.what();
    }
}

std::string MySQLConnection::getServerInfo() const {
    if (!m_connection) return "";
    try {
        return m_connection->getMetaData()->getDatabaseProductVersion();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getServerInfo error: " << e.what();
        return "";
    }
}

std::string MySQLConnection::getClientInfo() const {
    if (!m_driver) return "";
    try {
        return m_driver->getName();
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getClientInfo error: " << e.what();
        return "";
    }
}

uint64_t MySQLConnection::getLastInsertId() const {
    if (!m_connection || isClosed()) return 0;
    try {
        sql::Statement* stmt = m_connection->createStatement();
        sql::ResultSet* result = stmt->executeQuery("SELECT LAST_INSERT_ID()");
        uint64_t id = 0;
        if (result && result->next()) {
            id = result->getUInt64(1);
        }
        if (result) delete result;
        delete stmt;
        return id;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL getLastInsertId error: " << e.what();
        return 0;
    }
}

uint64_t MySQLConnection::executeUpdateAndGetId(const std::string& sql) {
    if (!m_connection || isClosed()) return 0;
    try {
        sql::Statement* stmt = m_connection->createStatement();
        int64_t affected = stmt->executeUpdate(sql);
        uint64_t id = 0;
        if (affected > 0) {
            id = getLastInsertId();
        }
        delete stmt;
        return id;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL executeUpdateAndGetId error: " << e.what();
        return 0;
    }
}

uint64_t MySQLConnection::executeUpdateAndGetId(MySQLPreparedStatement::ptr stmt) {
    if (!stmt) return 0;
    try {
        int64_t affected = stmt->executeUpdate();
        uint64_t id = 0;
        if (affected > 0) {
            id = getLastInsertId();
        }
        return id;
    } catch (const sql::SQLException& e) {
        SYLAR_LOG_ERROR(g_logger) << "MySQL executeUpdateAndGetId error: " << e.what();
        return 0;
    }
}

// ==================== MySQLConnectionPool 实现 ====================

MySQLConnectionPool::MySQLConnectionPool(const std::string& host, uint16_t port,
                                       const std::string& username, const std::string& password,
                                       const std::string& database,
                                       uint32_t maxConnections,
                                       uint32_t minConnections,
                                       uint32_t connectionTimeout,
                                       uint32_t idleTimeout)
    : m_host(host), m_port(port), m_username(username), m_password(password),
      m_database(database), m_maxConnections(maxConnections), 
      m_minConnections(minConnections), m_connectionTimeout(connectionTimeout),
      m_idleTimeout(idleTimeout), m_running(true) {
    
    // 创建初始连接
    for (uint32_t i = 0; i < m_minConnections; ++i) {
        if (!createConnection()) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to create initial connection " << i;
        }
    }
    
    // 启动定时清理任务
    m_cleanupTimer = sylar::IOManager::GetThis()->addTimer(
        30 * 1000, // 30秒检查一次
        [this]() {
            cleanupIdleConnections();
        },
        true // 循环执行
    );
    
    SYLAR_LOG_INFO(g_logger) << "MySQL connection pool created: " 
                            << host << ":" << port << "/" << database
                            << " (min:" << minConnections << ", max:" << maxConnections << ")";
}

MySQLConnectionPool::~MySQLConnectionPool() {
    closeAll();
}

MySQLConnection::ptr MySQLConnectionPool::getConnection() {
    // 第一步：先用读锁检查空闲连接（读操作，允许并发）
    sylar::RWMutex::ReadLock readLock(m_mutex);
    SYLAR_LOG_DEBUG(g_logger) << "active size:" << m_activeConnections.size() 
                             << "idle size: " << m_idleConnections.size();

    // 检查是否有空闲连接（读操作，无需写锁）
    if (!m_idleConnections.empty()) {
        // 第二步：确认需要修改连接池列表时，升级为写锁（仅在修改时加写锁）
        readLock.unlock();  // 先释放读锁，避免死锁（读锁升级写锁需先释放）
        sylar::RWMutex::WriteLock writeLock(m_mutex);

        // 再次检查空闲连接（防止释放读锁后被其他线程修改）
        if (!m_idleConnections.empty()) {
            auto conn = m_idleConnections.back();
            m_idleConnections.pop_back();  // 修改操作，需写锁

            // 第三步：连接有效性检查（耗时操作，释放写锁，避免阻塞其他线程）
            writeLock.unlock();

            if (conn->isClosed()) {
                SYLAR_LOG_DEBUG(g_logger) << "Idle connection is closed, reconnecting...";
                if (!conn->reconnect()) {
                    SYLAR_LOG_ERROR(g_logger) << "Failed to reconnect idle connection";
                    return nullptr;
                }
            }

            // 第四步：将连接加入活跃列表（再次加写锁）
            sylar::RWMutex::WriteLock writeLock2(m_mutex);
            m_activeConnections.push_back(conn);  // 修改操作，需写锁
            SYLAR_LOG_DEBUG(g_logger) << "Got connection from idle pool, active: " 
                                     << m_activeConnections.size() 
                                     << ", idle: " << m_idleConnections.size();
            return conn;
        }
        // 若释放读锁后空闲连接已被取空，重新走后续逻辑
    }

    // 无空闲连接，检查是否能创建新连接（读操作，用读锁）
    readLock.unlock();
    sylar::RWMutex::ReadLock readLock2(m_mutex);
    bool canCreate = (m_activeConnections.size() + m_idleConnections.size()) < m_maxConnections;
    readLock2.unlock();

    if (canCreate) {
        sylar::RWMutex::WriteLock writeLock(m_mutex);
        // 再次检查连接数（防止并发创建超上限）
        if ((m_activeConnections.size() + m_idleConnections.size()) < m_maxConnections) {
            auto conn = std::make_shared<MySQLConnection>();
            
            // 第五步：连接数据库（耗时操作，释放写锁）
            writeLock.unlock();
            if (conn->connect(m_host, m_port, m_username, m_password, m_database)) {
                // 连接成功后，加入活跃列表（加写锁）
                sylar::RWMutex::WriteLock writeLock2(m_mutex);
                m_activeConnections.push_back(conn);
                SYLAR_LOG_DEBUG(g_logger) << "Created new connection, active: " 
                                         << m_activeConnections.size();
                return conn;
            } else {
                SYLAR_LOG_ERROR(g_logger) << "Failed to create new connection";
                return nullptr;
            }
        }
    }

    // 连接池已满
    SYLAR_LOG_ERROR(g_logger) << "Connection pool is full, max connections: " << m_maxConnections;
    return nullptr;
}

void MySQLConnectionPool::returnConnection(MySQLConnection::ptr conn) {
    if (!conn) return;
    
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    // 从活跃连接列表中移除
    auto it = std::find(m_activeConnections.begin(), m_activeConnections.end(), conn);
    if (it != m_activeConnections.end()) {
        m_activeConnections.erase(it);
    }
    
    // 如果连接仍然有效，放回空闲池
    if (!conn->isClosed() && m_idleConnections.size() < m_maxConnections) {
        m_idleConnections.push_back(conn);
        SYLAR_LOG_DEBUG(g_logger) << "Returned connection to idle pool, active: " 
                                 << m_activeConnections.size() 
                                 << ", idle: " << m_idleConnections.size();
    } else {
        SYLAR_LOG_DEBUG(g_logger) << "Connection is invalid or pool is full, closing connection";
        conn->close();
    }
}

uint32_t MySQLConnectionPool::getActiveCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_activeConnections.size();
}

uint32_t MySQLConnectionPool::getIdleCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_idleConnections.size();
}

uint32_t MySQLConnectionPool::getTotalCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_activeConnections.size() + m_idleConnections.size();
}

void MySQLConnectionPool::closeAll() {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_running = false;
    
    // 关闭所有连接
    for (auto& conn : m_activeConnections) {
        conn->close();
    }
    for (auto& conn : m_idleConnections) {
        conn->close();
    }
    
    m_activeConnections.clear();
    m_idleConnections.clear();
    
    SYLAR_LOG_INFO(g_logger) << "All connections closed";
}

bool MySQLConnectionPool::createConnection() {
    auto conn = std::make_shared<MySQLConnection>();
    if (conn->connect(m_host, m_port, m_username, m_password, m_database)) {
        m_idleConnections.push_back(conn);
        return true;
    }
    return false;
}

void MySQLConnectionPool::cleanupIdleConnections() {
    if (!m_running) return;
    
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    // 清理空闲连接，保持最小连接数
    while (m_idleConnections.size() > m_minConnections) {
        auto conn = m_idleConnections.front();
        m_idleConnections.pop_front();
        conn->close();
        SYLAR_LOG_DEBUG(g_logger) << "Cleaned up idle connection";
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "Connection pool cleanup completed, active: " 
                             << m_activeConnections.size() 
                             << ", idle: " << m_idleConnections.size();
}

// ==================== MySQLStatementCache 实现 ====================

MySQLPreparedStatement::ptr MySQLStatementCache::getStatement(MySQLConnection::ptr conn, 
                                                             const std::string& sql) {
    if (!conn) return nullptr;
    
    uint64_t conn_id = reinterpret_cast<uint64_t>(conn.get());
    std::string sql_md5 = sylar::CryptoUtil::md5(sql);
    CacheKey key{conn_id, sql_md5};
    
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    auto& stmt_list = m_cache[key];
    if (!stmt_list.empty()) {
        auto stmt = stmt_list.back();
        stmt_list.pop_back();
        SYLAR_LOG_DEBUG(g_logger) << "Got prepared statement from cache";
        return stmt;
    }
    
    // 缓存中没有，创建新的预处理语句
    auto stmt = conn->prepareStatement(sql);
    if (stmt) {
        SYLAR_LOG_DEBUG(g_logger) << "Created new prepared statement";
    }
    return stmt;
}

void MySQLStatementCache::returnStatement(MySQLConnection::ptr conn, 
                                         const std::string& sql, 
                                         MySQLPreparedStatement::ptr stmt) {
    if (!conn || !stmt) return;
    
    uint64_t conn_id = reinterpret_cast<uint64_t>(conn.get());
    std::string sql_md5 = sylar::CryptoUtil::md5(sql);
    CacheKey key{conn_id, sql_md5};
    
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    // 限制每个SQL的最大缓存预处理语句数量
    auto& stmt_list = m_cache[key];
    if (stmt_list.size() < 10) { // 最多缓存10个相同的预处理语句
        stmt_list.push_back(stmt);
        SYLAR_LOG_DEBUG(g_logger) << "Returned prepared statement to cache, current size: " 
                                 << stmt_list.size();
    } else {
        SYLAR_LOG_DEBUG(g_logger) << "Prepared statement cache full, discarding statement";
    }
}

void MySQLStatementCache::clearConnection(MySQLConnection::ptr conn) {
    if (!conn) return;
    
    uint64_t conn_id = reinterpret_cast<uint64_t>(conn.get());
    
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    // 移除指定连接的所有预处理语句
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (it->first.connectionId == conn_id) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
    
    SYLAR_LOG_INFO(g_logger) << "Cleared all prepared statements for connection: " << conn_id;
}

void MySQLStatementCache::clear() {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_cache.clear();
    SYLAR_LOG_INFO(g_logger) << "Cleared all prepared statements from cache";
}

// ==================== MySQLManager 实现 ====================


bool MySQLManager::Init(const std::string& conf_path) {
    std::string path = conf_path;
    if(path.empty()) {
        path = sylar::EnvMgr::GetInstance()->getConfigPath();
    }
    
    try {
        YAML::Node root = YAML::LoadFile(path + "/mysql.yml");
        YAML::Node mysql_node = root["mysql"];
        if(!mysql_node) {
            SYLAR_LOG_ERROR(g_logger) << "mysql config not found in " << path + "/mysql.yml";
            return false;
        }
        for(auto it = mysql_node.begin(); it != mysql_node.end(); ++it) {
            std::string name = it->first.as<std::string>();
            YAML::Node node = it->second;
            
            MySQLConf conf;
            if(conf.loadFromYaml(node)) {
                uint32_t max_conn = node["max_conn"] ? node["max_conn"].as<uint32_t>() : 10;
                auto pool = std::make_shared<MySQLConnectionPool>(conf.host,conf.port,conf.user,conf.password,conf.database, max_conn,2,conf.timeout);
                MySQLManagerSingleton::GetInstance()->addPool(name, pool);
                
                SYLAR_LOG_INFO(g_logger) << "Init MySQL pool: " << name 
                                       << " " << conf.host << ":" << conf.port 
                                       << "/" << conf.database;

            }
        }
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load MySQL config error: " << e.what();
        return false;
    }
}

bool MySQLManager::addPool(const std::string& name, MySQLConnectionPool::ptr pool) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    
    if (m_pools.find(name) != m_pools.end()) {
        SYLAR_LOG_ERROR(g_logger) << "Connection pool already exists: " << name;
        return false;
    }
    
    m_pools[name] = pool;
    SYLAR_LOG_INFO(g_logger) << "Added connection pool: " << name;
    return true;
}

MySQLConnectionPool::ptr MySQLManager::getPool(const std::string& name) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    
    auto it = m_pools.find(name);
    if (it != m_pools.end()) {
        return it->second;
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "Connection pool not found: " << name;
    return nullptr;
}

MySQLConnection::ptr MySQLManager::getConnection(const std::string& poolName) {
    auto pool = getPool(poolName);
    if (!pool) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get connection pool: " << poolName;
        return nullptr;
    }
    
    return pool->getConnection();
}

} // namespace db
} // namespace sylar