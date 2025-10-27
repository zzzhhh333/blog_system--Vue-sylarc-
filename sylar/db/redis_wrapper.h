// redis_wrapper.h
#ifndef REDIS_WRAPPER_H
#define REDIS_WRAPPER_H

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <map>

namespace sylar {

/**
 * @brief Redis数据库封装类
 * 
 * 这个类封装了Redis hiredis C API，提供了简单易用的接口来操作Redis数据库。
 * 支持连接管理、字符串、哈希、列表、集合、有序集合、发布订阅、管道等操作。
 */
class RedisWrapper {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化Redis连接，但不建立实际连接。
     */
    RedisWrapper();
    
    /**
     * @brief 析构函数
     * 
     * 自动断开连接并清理资源。
     */
    ~RedisWrapper();
    
    // ==================== 连接管理 ====================
    
    /**
     * @brief 连接到Redis服务器
     * 
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @param timeout_ms 连接超时时间（毫秒）
     * @return true 连接成功
     * @return false 连接失败
     */
    bool connect(const std::string& host, int port, int timeout_ms = 0);
    
    /**
     * @brief 使用密码连接到Redis服务器
     * 
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @param password Redis密码
     * @param timeout_ms 连接超时时间（毫秒）
     * @return true 连接成功
     * @return false 连接失败
     */
    bool connectWithAuth(const std::string& host, int port, const std::string& password, int timeout_ms = 0);
    
    /**
     * @brief 断开Redis连接
     */
    void disconnect();
    
    /**
     * @brief 检查连接状态
     * 
     * @return true 连接正常
     * @return false 连接已断开
     */
    bool isConnected();
    
    /**
     * @brief 重新连接Redis服务器
     * 
     * @return true 重连成功
     * @return false 重连失败
     */
    bool reconnect();
    
    // ==================== 基础命令 ====================
    
    /**
     * @brief 执行Redis命令
     * 
     * @param command Redis命令字符串
     * @return std::string 命令执行结果
     */
    std::string execute(const std::string& command);
    
    // ==================== 键操作 ====================
    
    bool exists(const std::string& key);                 ///< 检查键是否存在
    bool del(const std::string& key);                    ///< 删除键
    bool expire(const std::string& key, int seconds);    ///< 设置键的过期时间
    bool persist(const std::string& key);                ///< 移除键的过期时间
    long long ttl(const std::string& key);               ///< 获取键的剩余生存时间
    
    // ==================== 字符串操作 ====================
    
    bool set(const std::string& key, const std::string& value);          ///< 设置字符串值
    bool setex(const std::string& key, int seconds, const std::string& value); ///< 设置带过期时间的字符串值
    std::string get(const std::string& key);                             ///< 获取字符串值
    long long incr(const std::string& key);                              ///< 将键的值增加1
    long long decr(const std::string& key);                              ///< 将键的值减少1
    
    // ==================== 哈希操作 ====================
    
    bool hset(const std::string& key, const std::string& field, const std::string& value); ///< 设置哈希字段值
    std::string hget(const std::string& key, const std::string& field);  ///< 获取哈希字段值
    bool hdel(const std::string& key, const std::string& field);         ///< 删除哈希字段
    bool hexists(const std::string& key, const std::string& field);      ///< 检查哈希字段是否存在
    std::map<std::string, std::string> hgetall(const std::string& key);  ///< 获取哈希的所有字段和值
    
    // ==================== 列表操作 ====================
    
    long long lpush(const std::string& key, const std::string& value);   ///< 从列表左侧插入值
    long long rpush(const std::string& key, const std::string& value);   ///< 从列表右侧插入值
    std::string lpop(const std::string& key);                            ///< 从列表左侧弹出值
    std::string rpop(const std::string& key);                            ///< 从列表右侧弹出值
    std::vector<std::string> lrange(const std::string& key, long long start, long long stop); ///< 获取列表指定范围内的元素
    long long llen(const std::string& key);                              ///< 获取列表长度
    
    // ==================== 集合操作 ====================
    
    long long sadd(const std::string& key, const std::string& member);   ///< 向集合添加成员
    long long srem(const std::string& key, const std::string& member);   ///< 从集合移除成员
    bool sismember(const std::string& key, const std::string& member);   ///< 检查成员是否在集合中
    std::vector<std::string> smembers(const std::string& key);           ///< 获取集合所有成员
    
    // ==================== 有序集合操作 ====================
    
    bool zadd(const std::string& key, double score, const std::string& member); ///< 向有序集合添加成员
    double zscore(const std::string& key, const std::string& member);    ///< 获取有序集合成员的分数
    std::vector<std::string> zrange(const std::string& key, long long start, long long stop); ///< 获取有序集合指定范围内的成员
    
    // ==================== 发布订阅 ====================
    
    bool publish(const std::string& channel, const std::string& message); ///< 发布消息到频道
    
    // ==================== 管道操作 ====================
    
    /**
     * @brief 开始管道模式
     * 
     * 在管道模式下，所有命令会被缓存而不是立即执行。
     */
    void startPipeline();
    
    /**
     * @brief 执行管道中的所有命令
     * 
     * @return std::vector<std::string> 所有命令的执行结果
     */
    std::vector<std::string> executePipeline();
    
    // ==================== 服务器信息 ====================
    
    /**
     * @brief 获取Redis服务器信息
     * 
     * @param section 信息区块，空字符串表示所有信息
     * @return std::string 服务器信息
     */
    std::string info(const std::string& section = "");
    
    // ==================== 连接测试 ====================
    
    /**
     * @brief 测试服务器连接
     * 
     * @return true 服务器响应正常
     * @return false 服务器无响应
     */
    bool ping();
    
    // ==================== 错误处理 ====================
    
    std::string getError();  ///< 获取错误信息
    bool hasError();         ///< 检查是否有错误

private:
    redisContext* context_;                  ///< Redis连接上下文
    std::vector<std::string> pipeline_commands_; ///< 管道模式下的命令缓存
    bool in_pipeline_;                       ///< 是否处于管道模式
    
    /**
     * @brief 执行Redis命令并返回原始回复
     * 
     * @param command Redis命令
     * @return redisReply* Redis回复对象
     */
    redisReply* executeCommand(const std::string& command);
    
    /**
     * @brief 解析Redis回复对象
     * 
     * @param reply Redis回复对象
     * @return std::string 解析后的字符串结果
     */
    std::string parseReply(redisReply* reply);
    
    /**
     * @brief 释放Redis回复对象
     * 
     * @param reply 要释放的回复对象
     */
    void freeReply(redisReply* reply);
};

}

#endif // REDIS_WRAPPER_H