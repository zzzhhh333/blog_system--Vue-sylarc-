#include "sylar/db/redis_connector.h"
#include <cstdarg>
#include <algorithm>

namespace sylar {
namespace db {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool RedisConf::loadFromYaml(const YAML::Node& node) {
    try {
        if (node["name"]) {
            name = node["name"].as<std::string>();
        }
        
        host = node["host"].as<std::string>(host);
        port = node["port"].as<uint32_t>(port);
        
        if (node["password"]) {
            password = node["password"].as<std::string>();
        }
        
        database = node["database"].as<uint32_t>(database);
        timeout = node["timeout"].as<uint32_t>(timeout);
        pool_size = node["pool_size"].as<uint32_t>(pool_size);
        pool_min_size = node["pool_min_size"].as<uint32_t>(pool_min_size);
        pool_max_size = node["pool_max_size"].as<uint32_t>(pool_max_size);
        pool_idle_timeout = node["pool_idle_timeout"].as<uint32_t>(pool_idle_timeout);
        
        // 加载哨兵配置
        if (node["master_name"]) {
            master_name = node["master_name"].as<std::string>();
        }
        
        if (node["sentinel_hosts"] && node["sentinel_hosts"].IsSequence()) {
            for (size_t i = 0; i < node["sentinel_hosts"].size(); ++i) {
                sentinel_hosts.push_back(node["sentinel_hosts"][i].as<std::string>());
            }
        }
        
        // 加载集群配置
        if (node["nodes"] && node["nodes"].IsSequence()) {
            for (size_t i = 0; i < node["nodes"].size(); ++i) {
                cluster_nodes.push_back(node["nodes"][i].as<std::string>());
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load redis config error: " << e.what();
        return false;
    }
}

RedisReply::RedisReply(redisReply* reply, bool ownsReply) 
    : m_reply(reply)
    , m_ownsReply(ownsReply) {
}

RedisReply::~RedisReply() {
    if (m_ownsReply && m_reply) {
        freeReplyObject(m_reply);
    }
}

RedisReplyType RedisReply::getType() const {
    if (!m_reply) {
        return RedisReplyType::NIL;
    }
    return static_cast<RedisReplyType>(m_reply->type);
}

bool RedisReply::isString() const {
    return getType() == RedisReplyType::STRING;
}

bool RedisReply::isArray() const {
    return getType() == RedisReplyType::ARRAY;
}

bool RedisReply::isInteger() const {
    return getType() == RedisReplyType::INTEGER;
}

bool RedisReply::isStatus() const {
    return getType() == RedisReplyType::STATUS;
}

bool RedisReply::isError() const {
    return getType() == RedisReplyType::ERROR;
}

bool RedisReply::isNil() const {
    return getType() == RedisReplyType::NIL;
}

std::string RedisReply::getString() const {
    if (!m_reply || !(m_reply->type == REDIS_REPLY_STRING || 
                      m_reply->type == REDIS_REPLY_STATUS || 
                      m_reply->type == REDIS_REPLY_ERROR)) {
        return "";
    }
    return std::string(m_reply->str, m_reply->len);
}

int64_t RedisReply::getInteger() const {
    if (!m_reply || m_reply->type != REDIS_REPLY_INTEGER) {
        return 0;
    }
    return m_reply->integer;
}

std::string RedisReply::getStatus() const {
    if (!m_reply || m_reply->type != REDIS_REPLY_STATUS) {
        return "";
    }
    return std::string(m_reply->str, m_reply->len);
}

std::string RedisReply::getError() const {
    if (!m_reply || m_reply->type != REDIS_REPLY_ERROR) {
        return "";
    }
    return std::string(m_reply->str, m_reply->len);
}

size_t RedisReply::getArraySize() const {
    if (!m_reply || m_reply->type != REDIS_REPLY_ARRAY) {
        return 0;
    }
    return m_reply->elements;
}

RedisReply::ptr RedisReply::getArrayElement(size_t index) const {
    if (!m_reply || m_reply->type != REDIS_REPLY_ARRAY || index >= m_reply->elements) {
        return nullptr;
    }
    return std::make_shared<RedisReply>(m_reply->element[index], false);
}

RedisConnection::RedisConnection() 
    : m_context(nullptr)
    , m_port(0)
    , m_timeout(0)
    , m_inTransaction(false) {
}

RedisConnection::~RedisConnection() {
    close();
}

bool RedisConnection::connect(const std::string& host, uint16_t port, uint32_t timeout) {
    m_host = host;
    m_port = port;
    m_timeout = timeout;
    
    timeval tv = {static_cast<time_t>(timeout), 0};
    m_context = redisConnectWithTimeout(host.c_str(), port, tv);
    
    if (!m_context || m_context->err) {
        if (m_context) {
            SYLAR_LOG_ERROR(g_logger) << "Redis connect error: " << m_context->errstr;
            redisFree(m_context);
            m_context = nullptr;
        } else {
            SYLAR_LOG_ERROR(g_logger) << "Redis connect error: can't allocate redis context";
        }
        return false;
    }
    
    return true;
}

bool RedisConnection::connect(const std::string& host, uint16_t port, const std::string& password, uint32_t timeout) {
    if (!connect(host, port, timeout)) {
        return false;
    }
    
    m_password = password;
    if (!password.empty() && !auth(password)) {
        close();
        return false;
    }
    
    return true;
}

void RedisConnection::close() {
    if (m_context) {
        redisFree(m_context);
        m_context = nullptr;
    }
    m_inTransaction = false;
}

bool RedisConnection::isConnected() const {
    return m_context && !m_context->err;
}

bool RedisConnection::reconnect() {
    close();
    if (m_password.empty()) {
        return connect(m_host, m_port, m_timeout);
    } else {
        return connect(m_host, m_port, m_password, m_timeout);
    }
}

RedisReply::ptr RedisConnection::executeCommand(const std::string& command) {
    if (!isConnected()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis connection is not connected";
        return nullptr;
    }
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_context, command.c_str()));
    if (!reply) {
        SYLAR_LOG_ERROR(g_logger) << "Redis command failed: " << m_context->errstr;
        return nullptr;
    }
    
    return std::make_shared<RedisReply>(reply);
}

RedisReply::ptr RedisConnection::executeCommand(const char* format, ...) {
    if (!isConnected()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis connection is not connected";
        return nullptr;
    }
    
    va_list ap;
    va_start(ap, format);
    redisReply* reply = reinterpret_cast<redisReply*>(redisvCommand(m_context, format, ap));
    va_end(ap);
    
    if (!reply) {
        SYLAR_LOG_ERROR(g_logger) << "Redis command failed: " << m_context->errstr;
        return nullptr;
    }
    
    return std::make_shared<RedisReply>(reply);
}

RedisReply::ptr RedisConnection::executeCommand(const char* format, va_list ap) {
    if (!isConnected()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis connection is not connected";
        return nullptr;
    }
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisvCommand(m_context, format, ap));
    if (!reply) {
        SYLAR_LOG_ERROR(g_logger) << "Redis command failed: " << m_context->errstr;
        return nullptr;
    }
    
    return std::make_shared<RedisReply>(reply);
}

RedisReply::ptr RedisConnection::executeCommand(const std::vector<std::string>& argv) {
    if (!isConnected()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis connection is not connected";
        return nullptr;
    }
    
    std::vector<const char*> args;
    std::vector<size_t> arg_lens;
    
    for (const auto& arg : argv) {
        args.push_back(arg.c_str());
        arg_lens.push_back(arg.length());
    }
    
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommandArgv(m_context, argv.size(), args.data(), arg_lens.data()));
    
    if (!reply) {
        SYLAR_LOG_ERROR(g_logger) << "Redis command failed: " << m_context->errstr;
        return nullptr;
    }
    
    return std::make_shared<RedisReply>(reply);
}

bool RedisConnection::auth(const std::string& password) {
    auto reply = executeCommand("AUTH %s", password.c_str());
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis auth failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return true;
}

bool RedisConnection::select(int database) {
    auto reply = executeCommand("SELECT %d", database);
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis select failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return true;
}

bool RedisConnection::ping() {
    auto reply = executeCommand("PING");
    if (!reply || reply->isError()) {
        return false;
    }
    return reply->isStatus() && reply->getStatus() == "PONG";
}

bool RedisConnection::set(const std::string& key, const std::string& value, int expire) {
    RedisReply::ptr reply;
    if (expire > 0) {
        reply = executeCommand("SET %s %s EX %d", key.c_str(), value.c_str(), expire);
    } else {
        reply = executeCommand("SET %s %s", key.c_str(), value.c_str());
    }
    
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis set failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return reply->isStatus() && reply->getStatus() == "OK";
}

std::string RedisConnection::get(const std::string& key) {
    auto reply = executeCommand("GET %s", key.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return "";
    }
    return reply->getString();
}

int64_t RedisConnection::del(const std::string& key) {
    auto reply = executeCommand("DEL %s", key.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

bool RedisConnection::exists(const std::string& key) {
    auto reply = executeCommand("EXISTS %s", key.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return reply->getInteger() > 0;
}

bool RedisConnection::expire(const std::string& key, int expire) {
    auto reply = executeCommand("EXPIRE %s %d", key.c_str(), expire);
    if (!reply || reply->isError()) {
        return false;
    }
    return reply->getInteger() > 0;
}

bool RedisConnection::setex(const std::string& key, int expire, const std::string& value) {
    auto reply = executeCommand("SETEX %s %d %s", key.c_str(), expire, value.c_str());
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis setex failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return reply->isStatus() && reply->getStatus() == "OK";
}

bool RedisConnection::setnx(const std::string& key, const std::string& value) {
    auto reply = executeCommand("SETNX %s %s", key.c_str(), value.c_str());
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis setnx failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return reply->getInteger() == 1;
}

bool RedisConnection::setnxex(const std::string& key, const std::string& value, int expire) {
    std::vector<std::string> argv = {"SET", key, value, "NX", "EX", std::to_string(expire)};
    auto reply = executeCommand(argv);
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis setnxex failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return !reply->isNil();
}

std::string RedisConnection::getset(const std::string& key, const std::string& value) {
    auto reply = executeCommand("GETSET %s %s", key.c_str(), value.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return "";
    }
    return reply->getString();
}

int64_t RedisConnection::strlen(const std::string& key) {
    auto reply = executeCommand("STRLEN %s", key.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::append(const std::string& key, const std::string& value) {
    auto reply = executeCommand("APPEND %s %s", key.c_str(), value.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::incr(const std::string& key) {
    auto reply = executeCommand("INCR %s", key.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::incrby(const std::string& key, int64_t increment) {
    auto reply = executeCommand("INCRBY %s %lld", key.c_str(), increment);
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

double RedisConnection::incrbyfloat(const std::string& key, double increment) {
    auto reply = executeCommand("INCRBYFLOAT %s %f", key.c_str(), increment);
    if (!reply || reply->isError()) {
        return 0.0;
    }
    
    std::string resultStr = reply->getString();
    try {
        return std::stod(resultStr);
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Parse incrbyfloat result failed: " << e.what();
        return 0.0;
    }
}

int64_t RedisConnection::decr(const std::string& key) {
    auto reply = executeCommand("DECR %s", key.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::decrby(const std::string& key, int64_t decrement) {
    auto reply = executeCommand("DECRBY %s %lld", key.c_str(), decrement);
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::ttl(const std::string& key) {
    auto reply = executeCommand("TTL %s", key.c_str());
    if (!reply || reply->isError()) {
        return -2;
    }
    return reply->getInteger();
}

int64_t RedisConnection::pttl(const std::string& key) {
    auto reply = executeCommand("PTTL %s", key.c_str());
    if (!reply || reply->isError()) {
        return -2;
    }
    return reply->getInteger();
}

bool RedisConnection::persist(const std::string& key) {
    auto reply = executeCommand("PERSIST %s", key.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return reply->getInteger() == 1;
}

bool RedisConnection::rename(const std::string& key, const std::string& newkey) {
    auto reply = executeCommand("RENAME %s %s", key.c_str(), newkey.c_str());
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis rename failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    return reply->isStatus() && reply->getStatus() == "OK";
}

std::vector<std::string> RedisConnection::keys(const std::string& pattern) {
    std::vector<std::string> result;
    auto reply = executeCommand("KEYS %s", pattern.c_str());
    if (!reply || reply->isError() || !reply->isArray()) {
        return result;
    }
    
    size_t size = reply->getArraySize();
    for (size_t i = 0; i < size; ++i) {
        auto element = reply->getArrayElement(i);
        if (element) {
            result.push_back(element->getString());
        }
    }
    
    return result;
}

bool RedisConnection::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto reply = executeCommand("HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return true;
}

std::string RedisConnection::hget(const std::string& key, const std::string& field) {
    auto reply = executeCommand("HGET %s %s", key.c_str(), field.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return "";
    }
    return reply->getString();
}

std::map<std::string, std::string> RedisConnection::hgetall(const std::string& key) {
    std::map<std::string, std::string> result;
    auto reply = executeCommand("HGETALL %s", key.c_str());
    if (!reply || reply->isError() || !reply->isArray()) {
        return result;
    }
    
    size_t size = reply->getArraySize();
    for (size_t i = 0; i < size; i += 2) {
        auto fieldReply = reply->getArrayElement(i);
        auto valueReply = reply->getArrayElement(i + 1);
        if (fieldReply && valueReply) {
            result[fieldReply->getString()] = valueReply->getString();
        }
    }
    
    return result;
}

int64_t RedisConnection::hdel(const std::string& key, const std::string& field) {
    auto reply = executeCommand("HDEL %s %s", key.c_str(), field.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::lpush(const std::string& key, const std::string& value) {
    auto reply = executeCommand("LPUSH %s %s", key.c_str(), value.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::rpush(const std::string& key, const std::string& value) {
    auto reply = executeCommand("RPUSH %s %s", key.c_str(), value.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

std::string RedisConnection::lpop(const std::string& key) {
    auto reply = executeCommand("LPOP %s", key.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return "";
    }
    return reply->getString();
}

std::string RedisConnection::rpop(const std::string& key) {
    auto reply = executeCommand("RPOP %s", key.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return "";
    }
    return reply->getString();
}

int64_t RedisConnection::llen(const std::string& key) {
    auto reply = executeCommand("LLEN %s", key.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::sadd(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SADD %s %s", key.c_str(), member.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

int64_t RedisConnection::srem(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SREM %s %s", key.c_str(), member.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

bool RedisConnection::sismember(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SISMEMBER %s %s", key.c_str(), member.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return reply->getInteger() > 0;
}

int64_t RedisConnection::zadd(const std::string& key, double score, const std::string& member) {
    auto reply = executeCommand("ZADD %s %f %s", key.c_str(), score, member.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

double RedisConnection::zscore(const std::string& key, const std::string& member) {
    auto reply = executeCommand("ZSCORE %s %s", key.c_str(), member.c_str());
    if (!reply || reply->isError() || reply->isNil()) {
        return 0.0;
    }
    
    std::string scoreStr = reply->getString();
    try {
        return std::stod(scoreStr);
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Parse zscore failed: " << e.what();
        return 0.0;
    }
}

bool RedisConnection::multi() {
    auto reply = executeCommand("MULTI");
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis multi failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    m_inTransaction = true;
    return true;
}

std::vector<RedisReply::ptr> RedisConnection::exec() {
    std::vector<RedisReply::ptr> results;
    auto reply = executeCommand("EXEC");
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis exec failed: " << (reply ? reply->getError() : "unknown error");
        m_inTransaction = false;
        return results;
    }
    
    if (reply->isArray()) {
        size_t size = reply->getArraySize();
        for (size_t i = 0; i < size; ++i) {
            results.push_back(reply->getArrayElement(i));
        }
    }
    
    m_inTransaction = false;
    return results;
}

bool RedisConnection::discard() {
    auto reply = executeCommand("DISCARD");
    if (!reply || reply->isError()) {
        SYLAR_LOG_ERROR(g_logger) << "Redis discard failed: " << (reply ? reply->getError() : "unknown error");
        return false;
    }
    m_inTransaction = false;
    return true;
}

int64_t RedisConnection::publish(const std::string& channel, const std::string& message) {
    auto reply = executeCommand("PUBLISH %s %s", channel.c_str(), message.c_str());
    if (!reply || reply->isError()) {
        return 0;
    }
    return reply->getInteger();
}

bool RedisConnection::subscribe(const std::string& channel) {
    auto reply = executeCommand("SUBSCRIBE %s", channel.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return true;
}

bool RedisConnection::unsubscribe(const std::string& channel) {
    auto reply = executeCommand("UNSUBSCRIBE %s", channel.c_str());
    if (!reply || reply->isError()) {
        return false;
    }
    return true;
}

RedisConnectionPool::RedisConnectionPool(const std::string& host, uint16_t port,
                                       const std::string& password, int database,
                                       uint32_t maxConnections,
                                       uint32_t minConnections,
                                       uint32_t connectionTimeout,
                                       uint32_t idleTimeout)
    : m_host(host)
    , m_port(port)
    , m_password(password)
    , m_database(database)
    , m_maxConnections(maxConnections)
    , m_minConnections(minConnections)
    , m_connectionTimeout(connectionTimeout)
    , m_idleTimeout(idleTimeout)
    , m_running(true) {
    
    // 创建最小连接数
    for (uint32_t i = 0; i < m_minConnections; ++i) {
        if (!createConnection()) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to create initial redis connection";
        }
    }
    
    // 启动清理定时器
    m_cleanupTimer = sylar::IOManager::GetThis()->addTimer(30 * 1000, 
        [this](){ cleanupIdleConnections(); }, true);
}

RedisConnectionPool::~RedisConnectionPool() {
    m_running = false;
    if (m_cleanupTimer) {
        m_cleanupTimer->cancel();
    }
    closeAll();
}

RedisConnection::ptr RedisConnectionPool::getConnection() {
    sylar::RWMutex::ReadLock lock_r(m_mutex);
    
    // 如果空闲连接池不为空，直接返回（释放读锁、短期写锁修改容器）
    if (!m_idleConnections.empty()) {
        lock_r.unlock();
        sylar::RWMutex::WriteLock lock(m_mutex);
        if (!m_idleConnections.empty()) {
            auto conn = m_idleConnections.front();
            m_idleConnections.pop_front();
            m_activeConnections.push_back(conn);
            return conn;
        }
        // 如果被其他线程抢走，继续下面逻辑
    }
    
    // 计算当前总连接数（在读锁下安全）
    size_t total = m_activeConnections.size() + m_idleConnections.size();
    // 如果没有达到最大连接数，尝试创建新连接（先释放读锁，createConnection 内部会加写锁）
    if (total < m_maxConnections) {
        lock_r.unlock();
        if (createConnection()) {
            // 新连接已安全放入 idle，取出并返回
            sylar::RWMutex::WriteLock lock(m_mutex);
            if (!m_idleConnections.empty()) {
                auto conn = m_idleConnections.front();
                m_idleConnections.pop_front();
                m_activeConnections.push_back(conn);
                return conn;
            }
        }
        // 创建失败或被其他线程抢占，回到读锁等待流程
        lock_r.lock();
    }
    
    // 等待空闲连接（简单实现，实际应使用条件变量）
    for (int i = 0; i < 10 && m_idleConnections.empty(); ++i) {
        lock_r.unlock();
        usleep(100000); // 100ms
        lock_r.lock();
    }
    
    if (!m_idleConnections.empty()) {
        lock_r.unlock();
        sylar::RWMutex::WriteLock lock(m_mutex);
        if (!m_idleConnections.empty()) {
            auto conn = m_idleConnections.front();
            m_idleConnections.pop_front();
            m_activeConnections.push_back(conn);
            return conn;
        }
    }
    
    SYLAR_LOG_ERROR(g_logger) << "No available redis connection in pool";
    return nullptr;
}

void RedisConnectionPool::returnConnection(RedisConnection::ptr conn) {
    if (!conn) {
        return;
    }
    
    sylar::RWMutex::ReadLock lock_r(m_mutex);
    
    // 从活跃连接中移除
    auto it = std::find(m_activeConnections.begin(), m_activeConnections.end(), conn);
    if (it != m_activeConnections.end()) {
        lock_r.unlock();
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_activeConnections.erase(it);
    }
    
    // 如果连接仍然有效，放回空闲连接池
    if (conn->isConnected()) {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_idleConnections.push_back(conn);
    } else {
        SYLAR_LOG_WARN(g_logger) << "Redis connection is invalid, discarding";
    }
}

uint32_t RedisConnectionPool::getActiveCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_activeConnections.size();
}

uint32_t RedisConnectionPool::getIdleCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_idleConnections.size();
}

uint32_t RedisConnectionPool::getTotalCount() const {
    sylar::RWMutex::ReadLock lock(m_mutex);
    return m_activeConnections.size() + m_idleConnections.size();
}

void RedisConnectionPool::closeAll() {
    sylar::RWMutex::WriteLock lock(m_mutex);
    for (auto& conn : m_idleConnections) {
        conn->close();
    }
    for (auto& conn : m_activeConnections) {
        conn->close();
    }
    m_idleConnections.clear();
    m_activeConnections.clear();
}

bool RedisConnectionPool::createConnection() {
    auto conn = std::make_shared<RedisConnection>();
    if (!conn->connect(m_host, m_port, m_password, m_connectionTimeout)) {
        return false;
    }
    
    if (m_database != 0 && !conn->select(m_database)) {
        conn->close();
        return false;
    }
    
    // 写入共享容器必须加写锁
    {
        sylar::RWMutex::WriteLock write(m_mutex);
        m_idleConnections.push_back(conn);
    }
    return true;
}

void RedisConnectionPool::cleanupIdleConnections() {
    // 1) 在写锁下尽快把待检查的连接移出池，避免其它线程同时使用同一连接
    std::vector<RedisConnection::ptr> to_check;
    {
        sylar::RWMutex::WriteLock write(m_mutex);
        // 优先移除超过最小数量的连接用于清理
        while (!m_idleConnections.empty() && m_idleConnections.size() > m_minConnections) {
            to_check.push_back(m_idleConnections.front());
            m_idleConnections.pop_front();
        }
        // 为了健康检查，也把当前剩余空闲连接做一次采样检查（可根据需要改为采样而非全部）
        // 这里选择把剩余全部也暂时移出，ping 后再放回，确保 ping 时不会有并发使用
        while (!m_idleConnections.empty()) {
            to_check.push_back(m_idleConnections.front());
            m_idleConnections.pop_front();
        }
    }

    if (to_check.empty()) {
        return;
    }

    // 2) 在锁外进行耗时的 ping 操作（会触发 IOManager 的事件等待），避免长时间持写锁
    std::vector<RedisConnection::ptr> healthy;
    healthy.reserve(to_check.size());
    for (auto &conn : to_check) {
        if (!conn) continue;
        bool ok = false;
        try {
            ok = conn->ping();
        } catch (...) {
            ok = false;
        }
        if (ok) {
            healthy.push_back(conn);
        } else {
            SYLAR_LOG_WARN(g_logger) << "Redis connection ping failed, closing";
            conn->close();
        }
    }

    // 3) 把健康的连接短时间写回池中，并保证最小连接数
    {
        sylar::RWMutex::WriteLock write(m_mutex);
        for (auto &c : healthy) {
            // 防止池超限：若已达到最大连接数则关闭多余连接
            if (m_idleConnections.size() + m_activeConnections.size() < m_maxConnections) {
                m_idleConnections.push_back(c);
            } else {
                c->close();
            }
        }
        // 如需保证最小连接数，尝试创建缺失的连接（createConnection 内部会加写锁）
        while (m_idleConnections.size() < m_minConnections) {
            // 注意：createConnection 会再次获取写锁，若你的 RWMutex 不允许重入请将此逻辑移到锁外并谨慎处理
            write.unlock();
            if (!createConnection()) {
                write.lock();
                break;
            }
            write.lock();
        }
    }
}

bool RedisManager::Init(const std::string& conf_path) {
    try {
        std::string actual_path = conf_path;
        if (actual_path.empty()) {
            // 如果没有指定配置文件路径，使用默认路径
            actual_path = sylar::EnvMgr::GetInstance()->getConfigPath();
        }
        
        // 加载YAML配置文件
        YAML::Node root = YAML::LoadFile(actual_path+"/redis.yml");
        YAML::Node redis_node = root["redis"];
        if (!redis_node) {
            SYLAR_LOG_ERROR(g_logger) << "No redis config found in " << actual_path+"/redis.yml";
            return false;  
        }
        // 遍历所有Redis配置
        for (auto it = redis_node.begin(); it != redis_node.end(); ++it) {
            std::string pool_name = it->first.as<std::string>();
            YAML::Node pool_config = it->second;
            
            RedisConf conf;
            conf.name = pool_name;
            
            if (!conf.loadFromYaml(pool_config)) {
                SYLAR_LOG_ERROR(g_logger) << "Load redis config failed for pool: " << pool_name;
                continue;
            }
            
            // 创建连接池
            RedisConnectionPool::ptr pool;
            
            if (!conf.sentinel_hosts.empty()) {
                // 哨兵模式 - 需要扩展连接池以支持哨兵
                SYLAR_LOG_WARN(g_logger) << "Sentinel mode not fully implemented for pool: " << pool_name;
                // 这里可以添加哨兵模式的具体实现
                continue;
            } else if (!conf.cluster_nodes.empty()) {
                // 集群模式 - 需要扩展连接池以支持集群
                SYLAR_LOG_WARN(g_logger) << "Cluster mode not fully implemented for pool: " << pool_name;
                // 这里可以添加集群模式的具体实现
                continue;
            } else {
                // 单机模式
                pool = std::make_shared<RedisConnectionPool>(
                    conf.host, conf.port, conf.password, conf.database,
                    conf.pool_min_size, conf.pool_max_size,
                    conf.timeout, conf.pool_idle_timeout);
            }
            
            if (db::RedisManagerSingleton::GetInstance()->addPool(pool_name, pool)) {
                SYLAR_LOG_INFO(g_logger) << "Add redis pool success: " << pool_name 
                    << " (" << conf.host << ":" << conf.port << "/" << conf.database << ")";
            } else {
                SYLAR_LOG_ERROR(g_logger) << "Add redis pool failed: " << pool_name;
            }
        }
        
        if (db::RedisManagerSingleton::GetInstance()->m_pools.empty()) {
            SYLAR_LOG_ERROR(g_logger) << "No valid redis pool configured";
            return false;
        }
        
        SYLAR_LOG_INFO(g_logger) << "Redis manager initialized with " << db::RedisManagerSingleton::GetInstance()->m_pools.size() << " pools";
        return true;
        
    } catch (const YAML::Exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load redis config file error: " << e.what();
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Redis manager init error: " << e.what();
    }
    
    return false;
}

bool RedisManager::addPool(const std::string& name, RedisConnectionPool::ptr pool) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    if (m_pools.find(name) != m_pools.end()) {
        return false;
    }
    m_pools[name] = pool;
    return true;
}

RedisConnectionPool::ptr RedisManager::getPool(const std::string& name) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_pools.find(name);
    if (it != m_pools.end()) {
        return it->second;
    }
    return nullptr;
}

RedisConnection::ptr RedisManager::getConnection(const std::string& poolName) {
    auto pool = getPool(poolName);
    if (!pool) {
        SYLAR_LOG_ERROR(g_logger) << "Redis pool not found: " << poolName;
        return nullptr;
    }
    return pool->getConnection();
}

} // namespace db
} // namespace sylar