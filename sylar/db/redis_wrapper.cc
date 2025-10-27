// redis_wrapper.cpp
#include "redis_wrapper.h"
#include <sstream>
#include "sylar/log.h"  // 添加Sylar日志头文件

namespace sylar {

// 定义Redis专用的日志器
static sylar::Logger::ptr g_redis_logger = SYLAR_LOG_NAME("redis");

/**
 * @brief 构造函数
 */
RedisWrapper::RedisWrapper() : context_(nullptr), in_pipeline_(false) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Redis wrapper initialized";
}

/**
 * @brief 析构函数
 */
RedisWrapper::~RedisWrapper() {
    disconnect();
    SYLAR_LOG_DEBUG(g_redis_logger) << "Redis wrapper destroyed";
}

/**
 * @brief 连接到Redis服务器
 */
bool RedisWrapper::connect(const std::string& host, int port, int timeout_ms) {
    SYLAR_LOG_INFO(g_redis_logger) << "Connecting to Redis: " << host << ":" << port 
                                  << ", timeout: " << timeout_ms << "ms";
    
    disconnect();
    
    // 设置连接超时
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    
    // 建立连接
    context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    
    // 检查连接是否成功
    bool success = context_ && !context_->err;
    if (success) {
        SYLAR_LOG_INFO(g_redis_logger) << "Redis connected successfully";
    } else {
        std::string error_msg = context_ ? context_->errstr : "Unknown error";
        SYLAR_LOG_ERROR(g_redis_logger) << "Redis connection failed: " << error_msg;
    }
    
    return success;
}

/**
 * @brief 使用密码连接到Redis服务器
 */
bool RedisWrapper::connectWithAuth(const std::string& host, int port, const std::string& password, int timeout_ms) {
    SYLAR_LOG_INFO(g_redis_logger) << "Connecting to Redis with auth: " << host << ":" << port;
    
    // 先建立连接
    if (!connect(host, port, timeout_ms)) {
        return false;
    }
    
    // 如果有密码，进行认证
    if (!password.empty()) {
        std::string auth_cmd = "AUTH " + password;
        redisReply* reply = executeCommand(auth_cmd);
        bool success = reply && reply->type != REDIS_REPLY_ERROR;
        
        if (success) {
            SYLAR_LOG_INFO(g_redis_logger) << "Redis authentication successful";
        } else {
            SYLAR_LOG_ERROR(g_redis_logger) << "Redis authentication failed";
        }
        
        freeReply(reply);
        return success;
    }
    
    SYLAR_LOG_DEBUG(g_redis_logger) << "No password provided, skip authentication";
    return true;
}

/**
 * @brief 断开Redis连接
 */
void RedisWrapper::disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
        SYLAR_LOG_INFO(g_redis_logger) << "Redis disconnected";
    }
    pipeline_commands_.clear();
    in_pipeline_ = false;
}

/**
 * @brief 检查连接状态
 */
bool RedisWrapper::isConnected() {
    bool connected = context_ && !context_->err;
    if (!connected) {
        SYLAR_LOG_WARN(g_redis_logger) << "Redis connection check failed";
    }
    return connected;
}

/**
 * @brief 重新连接Redis服务器
 */
bool RedisWrapper::reconnect() {
    SYLAR_LOG_INFO(g_redis_logger) << "Attempting to reconnect Redis";
    if (!context_) {
        SYLAR_LOG_ERROR(g_redis_logger) << "Cannot reconnect, context is null";
        return false;
    }
    
    bool success = redisReconnect(context_) == REDIS_OK;
    if (success) {
        SYLAR_LOG_INFO(g_redis_logger) << "Redis reconnected successfully";
    } else {
        SYLAR_LOG_ERROR(g_redis_logger) << "Redis reconnect failed: " << getError();
    }
    
    return success;
}

/**
 * @brief 执行Redis命令
 */
std::string RedisWrapper::execute(const std::string& command) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Execute Redis command: " << command;
    
    // 如果处于管道模式，缓存命令但不执行
    if (in_pipeline_) {
        pipeline_commands_.push_back(command);
        SYLAR_LOG_DEBUG(g_redis_logger) << "Command added to pipeline, current size: " << pipeline_commands_.size();
        return "";
    }
    
    // 执行命令并返回结果
    redisReply* reply = executeCommand(command);
    std::string result = parseReply(reply);
    
    if (reply && reply->type == REDIS_REPLY_ERROR) {
        SYLAR_LOG_ERROR(g_redis_logger) << "Redis command failed: " << (reply->str ? reply->str : "Unknown error")
                                       << ", command: " << command;
    } else {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Command result: " << result;
    }
    
    freeReply(reply);
    return result;
}

/**
 * @brief 检查键是否存在
 */
bool RedisWrapper::exists(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Check key exists: " << key;
    std::string result = execute("EXISTS " + key);
    bool exists = result == "1";
    SYLAR_LOG_DEBUG(g_redis_logger) << "Key " << key << " exists: " << exists;
    return exists;
}

/**
 * @brief 删除键
 */
bool RedisWrapper::del(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Delete key: " << key;
    std::string result = execute("DEL " + key);
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Key deleted: " << key;
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Key not found or delete failed: " << key;
    }
    return success;
}

/**
 * @brief 设置键的过期时间
 */
bool RedisWrapper::expire(const std::string& key, int seconds) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set key expire: " << key << ", seconds: " << seconds;
    std::string result = execute("EXPIRE " + key + " " + std::to_string(seconds));
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Key expire set: " << key;
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Set key expire failed: " << key;
    }
    return success;
}

/**
 * @brief 移除键的过期时间
 */
bool RedisWrapper::persist(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Persist key: " << key;
    std::string result = execute("PERSIST " + key);
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Key persist set: " << key;
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Set key persist failed: " << key;
    }
    return success;
}

/**
 * @brief 获取键的剩余生存时间
 */
long long RedisWrapper::ttl(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Get key TTL: " << key;
    std::string result = execute("TTL " + key);
    long long ttl = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Key " << key << " TTL: " << ttl;
    return ttl;
}

/**
 * @brief 设置字符串值
 */
bool RedisWrapper::set(const std::string& key, const std::string& value) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set string: " << key << " = " << value;
    std::string result = execute("SET " + key + " " + value);
    bool success = result == "OK";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "String set successfully: " << key;
    } else {
        SYLAR_LOG_ERROR(g_redis_logger) << "Set string failed: " << key;
    }
    return success;
}

/**
 * @brief 设置带过期时间的字符串值
 */
bool RedisWrapper::setex(const std::string& key, int seconds, const std::string& value) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set string with expire: " << key << " = " << value << ", seconds: " << seconds;
    std::string result = execute("SETEX " + key + " " + std::to_string(seconds) + " " + value);
    bool success = result == "OK";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "String with expire set successfully: " << key;
    } else {
        SYLAR_LOG_ERROR(g_redis_logger) << "Set string with expire failed: " << key;
    }
    return success;
}

/**
 * @brief 获取字符串值
 */
std::string RedisWrapper::get(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Get string: " << key;
    std::string result = execute("GET " + key);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Get string result: " << key << " = " << result;
    return result;
}

/**
 * @brief 将键的值增加1
 */
long long RedisWrapper::incr(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Increment key: " << key;
    std::string result = execute("INCR " + key);
    long long value = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Increment result: " << key << " = " << value;
    return value;
}

/**
 * @brief 将键的值减少1
 */
long long RedisWrapper::decr(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Decrement key: " << key;
    std::string result = execute("DECR " + key);
    long long value = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Decrement result: " << key << " = " << value;
    return value;
}

/**
 * @brief 设置哈希字段值
 */
bool RedisWrapper::hset(const std::string& key, const std::string& field, const std::string& value) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash set: " << key << "." << field << " = " << value;
    std::string result = execute("HSET " + key + " " + field + " " + value);
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Hash field set successfully: " << key << "." << field;
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Hash field set failed or field exists: " << key << "." << field;
    }
    return success;
}

/**
 * @brief 获取哈希字段值
 */
std::string RedisWrapper::hget(const std::string& key, const std::string& field) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash get: " << key << "." << field;
    std::string result = execute("HGET " + key + " " + field);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash get result: " << key << "." << field << " = " << result;
    return result;
}

/**
 * @brief 删除哈希字段
 */
bool RedisWrapper::hdel(const std::string& key, const std::string& field) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash delete: " << key << "." << field;
    std::string result = execute("HDEL " + key + " " + field);
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Hash field deleted: " << key << "." << field;
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Hash field not found or delete failed: " << key << "." << field;
    }
    return success;
}

/**
 * @brief 检查哈希字段是否存在
 */
bool RedisWrapper::hexists(const std::string& key, const std::string& field) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash exists: " << key << "." << field;
    std::string result = execute("HEXISTS " + key + " " + field);
    bool exists = result == "1";
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash field " << key << "." << field << " exists: " << exists;
    return exists;
}

/**
 * @brief 获取哈希的所有字段和值
 */
std::map<std::string, std::string> RedisWrapper::hgetall(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash get all: " << key;
    std::map<std::string, std::string> result;
    redisReply* reply = executeCommand("HGETALL " + key);
    
    // 解析哈希回复（字段和值交替出现）
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Hash " << key << " has " << reply->elements << " elements";
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i + 1 < reply->elements) {
                std::string field = reply->element[i]->str ? reply->element[i]->str : "";
                std::string value = reply->element[i+1]->str ? reply->element[i+1]->str : "";
                result[field] = value;
                SYLAR_LOG_DEBUG(g_redis_logger) << "Hash field: " << field << " = " << value;
            }
        }
    } else if (reply && reply->type == REDIS_REPLY_ERROR) {
        SYLAR_LOG_ERROR(g_redis_logger) << "HGETALL failed: " << (reply->str ? reply->str : "Unknown error");
    }
    
    freeReply(reply);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Hash get all completed, found " << result.size() << " fields";
    return result;
}

/**
 * @brief 从列表左侧插入值
 */
long long RedisWrapper::lpush(const std::string& key, const std::string& value) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List left push: " << key << " <- " << value;
    std::string result = execute("LPUSH " + key + " " + value);
    long long length = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "List left push result, new length: " << length;
    return length;
}

/**
 * @brief 从列表右侧插入值
 */
long long RedisWrapper::rpush(const std::string& key, const std::string& value) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List right push: " << key << " -> " << value;
    std::string result = execute("RPUSH " + key + " " + value);
    long long length = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "List right push result, new length: " << length;
    return length;
}

/**
 * @brief 从列表左侧弹出值
 */
std::string RedisWrapper::lpop(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List left pop: " << key;
    std::string result = execute("LPOP " + key);
    SYLAR_LOG_DEBUG(g_redis_logger) << "List left pop result: " << result;
    return result;
}

/**
 * @brief 从列表右侧弹出值
 */
std::string RedisWrapper::rpop(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List right pop: " << key;
    std::string result = execute("RPOP " + key);
    SYLAR_LOG_DEBUG(g_redis_logger) << "List right pop result: " << result;
    return result;
}

/**
 * @brief 获取列表指定范围内的元素
 */
std::vector<std::string> RedisWrapper::lrange(const std::string& key, long long start, long long stop) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List range: " << key << " [" << start << ":" << stop << "]";
    std::vector<std::string> result;
    std::string cmd = "LRANGE " + key + " " + std::to_string(start) + " " + std::to_string(stop);
    redisReply* reply = executeCommand(cmd);
    
    // 解析数组回复
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "List range found " << reply->elements << " elements";
        for (size_t i = 0; i < reply->elements; i++) {
            if (reply->element[i]->str) {
                result.push_back(reply->element[i]->str);
                SYLAR_LOG_DEBUG(g_redis_logger) << "List element [" << i << "]: " << reply->element[i]->str;
            }
        }
    }
    
    freeReply(reply);
    return result;
}

/**
 * @brief 获取列表长度
 */
long long RedisWrapper::llen(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "List length: " << key;
    std::string result = execute("LLEN " + key);
    long long length = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "List " << key << " length: " << length;
    return length;
}

/**
 * @brief 向集合添加成员
 */
long long RedisWrapper::sadd(const std::string& key, const std::string& member) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set add: " << key << " += " << member;
    std::string result = execute("SADD " + key + " " + member);
    long long count = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set add result, added " << count << " members";
    return count;
}

/**
 * @brief 从集合移除成员
 */
long long RedisWrapper::srem(const std::string& key, const std::string& member) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set remove: " << key << " -= " << member;
    std::string result = execute("SREM " + key + " " + member);
    long long count = std::stoll(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set remove result, removed " << count << " members";
    return count;
}

/**
 * @brief 检查成员是否在集合中
 */
bool RedisWrapper::sismember(const std::string& key, const std::string& member) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set is member: " << key << " contains " << member;
    std::string result = execute("SISMEMBER " + key + " " + member);
    bool is_member = result == "1";
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set member check: " << is_member;
    return is_member;
}

/**
 * @brief 获取集合所有成员
 */
std::vector<std::string> RedisWrapper::smembers(const std::string& key) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Set members: " << key;
    std::vector<std::string> result;
    redisReply* reply = executeCommand("SMEMBERS " + key);
    
    // 解析数组回复
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Set " << key << " has " << reply->elements << " members";
        for (size_t i = 0; i < reply->elements; i++) {
            if (reply->element[i]->str) {
                result.push_back(reply->element[i]->str);
                SYLAR_LOG_DEBUG(g_redis_logger) << "Set member: " << reply->element[i]->str;
            }
        }
    }
    
    freeReply(reply);
    return result;
}

/**
 * @brief 向有序集合添加成员
 */
bool RedisWrapper::zadd(const std::string& key, double score, const std::string& member) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set add: " << key << " [" << score << "] = " << member;
    std::string cmd = "ZADD " + key + " " + std::to_string(score) + " " + member;
    std::string result = execute(cmd);
    bool success = result == "1";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set add successful";
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Sorted set add failed or member exists";
    }
    return success;
}

/**
 * @brief 获取有序集合成员的分数
 */
double RedisWrapper::zscore(const std::string& key, const std::string& member) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set score: " << key << " -> " << member;
    std::string result = execute("ZSCORE " + key + " " + member);
    double score = result.empty() ? 0.0 : std::stod(result);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set score result: " << score;
    return score;
}

/**
 * @brief 获取有序集合指定范围内的成员
 */
std::vector<std::string> RedisWrapper::zrange(const std::string& key, long long start, long long stop) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set range: " << key << " [" << start << ":" << stop << "]";
    std::vector<std::string> result;
    std::string cmd = "ZRANGE " + key + " " + std::to_string(start) + " " + std::to_string(stop);
    redisReply* reply = executeCommand(cmd);
    
    // 解析数组回复
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set range found " << reply->elements << " members";
        for (size_t i = 0; i < reply->elements; i++) {
            if (reply->element[i]->str) {
                result.push_back(reply->element[i]->str);
                SYLAR_LOG_DEBUG(g_redis_logger) << "Sorted set member: " << reply->element[i]->str;
            }
        }
    }
    
    freeReply(reply);
    return result;
}

/**
 * @brief 发布消息到频道
 */
bool RedisWrapper::publish(const std::string& channel, const std::string& message) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Publish to channel: " << channel << " -> " << message;
    std::string result = execute("PUBLISH " + channel + " " + message);
    bool success = !result.empty();
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Publish successful, " << result << " subscribers received";
    } else {
        SYLAR_LOG_WARN(g_redis_logger) << "Publish failed";
    }
    return success;
}

/**
 * @brief 开始管道模式
 */
void RedisWrapper::startPipeline() {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Start Redis pipeline";
    in_pipeline_ = true;
    pipeline_commands_.clear();
}

/**
 * @brief 执行管道中的所有命令
 */
std::vector<std::string> RedisWrapper::executePipeline() {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Execute Redis pipeline, command count: " << pipeline_commands_.size();
    
    std::vector<std::string> results;
    
    if (!in_pipeline_ || pipeline_commands_.empty()) {
        SYLAR_LOG_WARN(g_redis_logger) << "No pipeline commands to execute";
        in_pipeline_ = false;
        return results;
    }
    
    // 依次执行所有缓存的命令
    for (const auto& cmd : pipeline_commands_) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Pipeline executing: " << cmd;
        redisReply* reply = executeCommand(cmd);
        std::string result = parseReply(reply);
        results.push_back(result);
        
        if (reply && reply->type == REDIS_REPLY_ERROR) {
            SYLAR_LOG_ERROR(g_redis_logger) << "Pipeline command failed: " << (reply->str ? reply->str : "Unknown error")
                                           << ", command: " << cmd;
        }
        
        freeReply(reply);
    }
    
    // 清理管道状态
    pipeline_commands_.clear();
    in_pipeline_ = false;
    
    SYLAR_LOG_DEBUG(g_redis_logger) << "Pipeline execution completed, result count: " << results.size();
    return results;
}

/**
 * @brief 获取Redis服务器信息
 */
std::string RedisWrapper::info(const std::string& section) {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Get Redis info, section: " << (section.empty() ? "all" : section);
    std::string cmd = "INFO";
    if (!section.empty()) {
        cmd += " " + section;
    }
    std::string result = execute(cmd);
    SYLAR_LOG_DEBUG(g_redis_logger) << "Redis info received, length: " << result.length();
    return result;
}

/**
 * @brief 测试服务器连接
 */
bool RedisWrapper::ping() {
    SYLAR_LOG_DEBUG(g_redis_logger) << "Ping Redis server";
    std::string result = execute("PING");
    bool success = result == "PONG";
    if (success) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Redis ping successful";
    } else {
        SYLAR_LOG_ERROR(g_redis_logger) << "Redis ping failed, result: " << result;
    }
    return success;
}

/**
 * @brief 获取错误信息
 */
std::string RedisWrapper::getError() {
    std::string error = "Not connected";
    if (context_) {
        error = context_->errstr;
        if (!error.empty()) {
            SYLAR_LOG_DEBUG(g_redis_logger) << "Redis error: " << error;
        }
    }
    return error;
}

/**
 * @brief 检查是否有错误
 */
bool RedisWrapper::hasError() {
    bool has_error = context_ && context_->err;
    if (has_error) {
        SYLAR_LOG_DEBUG(g_redis_logger) << "Redis has error: " << getError();
    }
    return has_error;
}

/**
 * @brief 执行Redis命令并返回原始回复
 */
redisReply* RedisWrapper::executeCommand(const std::string& command) {
    if (!context_) {
        SYLAR_LOG_ERROR(g_redis_logger) << "Cannot execute command, Redis not connected: " << command;
        return nullptr;
    }
    
    SYLAR_LOG_DEBUG(g_redis_logger) << "Execute raw command: " << command;
    redisReply* reply = (redisReply*)redisCommand(context_, command.c_str());
    
    if (!reply) {
        SYLAR_LOG_ERROR(g_redis_logger) << "Execute command failed, reply is null: " << command;
    }
    
    return reply;
}

/**
 * @brief 解析Redis回复对象
 */
std::string RedisWrapper::parseReply(redisReply* reply) {
    if (!reply) {
        SYLAR_LOG_WARN(g_redis_logger) << "Parse reply failed, reply is null";
        return "";
    }
    
    // 根据回复类型解析结果
    std::string result;
    switch (reply->type) {
        case REDIS_REPLY_STRING:    // 字符串回复
        case REDIS_REPLY_STATUS:    // 状态回复
            result = reply->str ? reply->str : "";
            break;
        case REDIS_REPLY_INTEGER:   // 整数回复
            result = std::to_string(reply->integer);
            break;
        case REDIS_REPLY_NIL:       // 空回复
            result = "";
            break;
        case REDIS_REPLY_ARRAY:     // 数组回复
            // 对于数组类型，返回第一个元素或空字符串
            if (reply->elements > 0 && reply->element[0]->str) {
                result = reply->element[0]->str;
            } else {
                result = "";
            }
            break;
        case REDIS_REPLY_ERROR:     // 错误回复
            result = "";
            break;
        default:
            result = "";
            SYLAR_LOG_WARN(g_redis_logger) << "Unknown reply type: " << reply->type;
            break;
    }
    
    SYLAR_LOG_DEBUG(g_redis_logger) << "Parse reply, type: " << reply->type << ", result: " << result;
    return result;
}

/**
 * @brief 释放Redis回复对象
 */
void RedisWrapper::freeReply(redisReply* reply) {
    if (reply) {
        freeReplyObject(reply);
        SYLAR_LOG_DEBUG(g_redis_logger) << "Redis reply freed";
    }
}

} // namespace sylar