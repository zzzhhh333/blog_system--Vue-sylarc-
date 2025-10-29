#ifndef SYLAR_REDIS_CONNECTOR_H
#define SYLAR_REDIS_CONNECTOR_H

#include <memory>
#include <string>
#include <deque>
#include <map>
#include <vector>
#include <unordered_map>
#include <hiredis/hiredis.h>
#include "sylar/singleton.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include "sylar/iomanager.h"
#include "sylar/env.h"
#include "sylar/config.h"

namespace sylar {
namespace db {

/**
 * @brief Redis异常类
 * 封装Redis操作过程中可能抛出的异常，提供统一的异常处理接口
 */
class RedisException : public std::exception {
public:
    /**
     * @brief 构造函数
     * @param message 异常信息
     */
    explicit RedisException(const std::string& message) : m_message(message) {}
    
    /**
     * @brief 获取异常信息
     * @return 异常描述字符串
     */
    const char* what() const noexcept override { return m_message.c_str(); }
    
private:
    std::string m_message;  ///< 异常信息
};

/**
 * @brief Redis连接配置
 */
class RedisConf {
public:
    std::string name;               ///< 配置名称
    std::string host = "127.0.0.1"; ///< 主机地址
    uint32_t port = 6379;           ///< 端口号
    std::string password;           ///< 密码
    uint32_t database = 0;          ///< 数据库索引
    uint32_t timeout = 30;          ///< 连接超时时间(秒)
    uint32_t pool_size = 10;        ///< 连接池大小
    uint32_t pool_min_size = 2;     ///< 连接池最小连接数
    uint32_t pool_max_size = 20;    ///< 连接池最大连接数
    uint32_t pool_idle_timeout = 60;///< 空闲连接超时时间(秒)
    
    // 哨兵模式配置
    std::string master_name;        ///< 主节点名称
    std::vector<std::string> sentinel_hosts; ///< 哨兵节点列表
    
    // 集群模式配置  
    std::vector<std::string> cluster_nodes;  ///< 集群节点列表
    
    /**
     * @brief 从YAML节点加载配置
     * @param node YAML节点
     * @return 加载成功返回true，失败返回false
     */
    bool loadFromYaml(const YAML::Node& node);
};
/**
 * @brief Redis回复类型
 */
enum class RedisReplyType {
    NIL = REDIS_REPLY_NIL,           ///< 空回复
    STRING = REDIS_REPLY_STRING,     ///< 字符串回复
    ARRAY = REDIS_REPLY_ARRAY,       ///< 数组回复
    INTEGER = REDIS_REPLY_INTEGER,   ///< 整数回复
    STATUS = REDIS_REPLY_STATUS,     ///< 状态回复
    ERROR = REDIS_REPLY_ERROR        ///< 错误回复
};

/**
 * @brief Redis回复封装
 * 封装Redis命令的回复，提供类型安全的数据访问方法
 */
class RedisReply {
public:
    typedef std::shared_ptr<RedisReply> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param reply Redis回复指针
     * @param ownsReply 是否拥有回复对象的所有权
     */
    explicit RedisReply(redisReply* reply, bool ownsReply = true);
    
    /**
     * @brief 析构函数，自动释放资源
     */
    ~RedisReply();
    
    /**
     * @brief 获取回复类型
     * @return 回复类型枚举
     */
    RedisReplyType getType() const;
    
    /**
     * @brief 检查回复是否为字符串类型
     * @return 如果是字符串类型返回true，否则返回false
     */
    bool isString() const;
    
    /**
     * @brief 检查回复是否为数组类型
     * @return 如果是数组类型返回true，否则返回false
     */
    bool isArray() const;
    
    /**
     * @brief 检查回复是否为整数类型
     * @return 如果是整数类型返回true，否则返回false
     */
    bool isInteger() const;
    
    /**
     * @brief 检查回复是否为状态类型
     * @return 如果是状态类型返回true，否则返回false
     */
    bool isStatus() const;
    
    /**
     * @brief 检查回复是否为错误类型
     * @return 如果是错误类型返回true，否则返回false
     */
    bool isError() const;
    
    /**
     * @brief 检查回复是否为空
     * @return 如果回复为空返回true，否则返回false
     */
    bool isNil() const;
    
    // 数据获取方法
    
    /**
     * @brief 获取字符串值
     * @return 字符串值，如果回复不是字符串类型返回空字符串
     */
    std::string getString() const;
    
    /**
     * @brief 获取整数值
     * @return 整数值，如果回复不是整数类型返回0
     */
    int64_t getInteger() const;
    
    /**
     * @brief 获取状态字符串
     * @return 状态字符串，如果回复不是状态类型返回空字符串
     */
    std::string getStatus() const;
    
    /**
     * @brief 获取错误信息
     * @return 错误信息，如果回复不是错误类型返回空字符串
     */
    std::string getError() const;
    
    /**
     * @brief 获取数组元素数量
     * @return 数组元素数量，如果回复不是数组类型返回0
     */
    size_t getArraySize() const;
    
    /**
     * @brief 获取数组元素
     * @param index 元素索引
     * @return Redis回复指针，如果索引越界或不是数组类型返回nullptr
     */
    RedisReply::ptr getArrayElement(size_t index) const;
    
    /**
     * @brief 获取原始Redis回复指针
     * @return Redis回复指针
     */
    redisReply* getRawReply() const { return m_reply; }
    
private:
    redisReply* m_reply;     ///< Redis回复指针
    bool m_ownsReply;        ///< 是否拥有回复对象的所有权
};

/**
 * @brief Redis数据库连接
 * 封装Redis数据库连接，提供连接管理、命令执行和事务支持
 */
class RedisConnection {
public:
    typedef std::shared_ptr<RedisConnection> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     */
    RedisConnection();
    
    /**
     * @brief 析构函数，自动关闭连接
     */
    ~RedisConnection();
    
    /**
     * @brief 连接到Redis服务器
     * @param host 服务器主机地址
     * @param port 服务器端口
     * @param timeout 连接超时时间(秒)
     * @return 连接成功返回true，失败返回false
     */
    bool connect(const std::string& host, uint16_t port, uint32_t timeout = 30);
    
    /**
     * @brief 连接到Redis服务器（带密码认证）
     * @param host 服务器主机地址
     * @param port 服务器端口
     * @param password 密码
     * @param timeout 连接超时时间(秒)
     * @return 连接成功返回true，失败返回false
     */
    bool connect(const std::string& host, uint16_t port, const std::string& password, uint32_t timeout = 30);
    
    /**
     * @brief 关闭连接
     */
    void close();
    
    /**
     * @brief 检查连接是否有效
     * @return 连接有效返回true，否则返回false
     */
    bool isConnected() const;
    
    /**
     * @brief 重新连接
     * @return 重连成功返回true，失败返回false
     */
    bool reconnect();
    
    // 基本命令执行
    
    /**
     * @brief 执行Redis命令
     * @param command 命令字符串
     * @return 命令回复指针，执行失败返回nullptr
     */
    RedisReply::ptr executeCommand(const std::string& command);
    
    /**
     * @brief 执行Redis命令（带格式化参数）
     * @param format 命令格式字符串
     * @param ... 可变参数
     * @return 命令回复指针，执行失败返回nullptr
     */
    RedisReply::ptr executeCommand(const char* format, ...);
    
    /**
     * @brief 执行Redis命令（使用va_list）
     * @param format 命令格式字符串
     * @param ap 可变参数列表
     * @return 命令回复指针，执行失败返回nullptr
     */
    RedisReply::ptr executeCommand(const char* format, va_list ap);
    
    /**
     * @brief 执行Redis命令（字符串向量参数）
     * @param argv 命令参数向量
     * @param argc 参数数量
     * @return 命令回复指针，执行失败返回nullptr
     */
    RedisReply::ptr executeCommand(const std::vector<std::string>& argv);
    
    // 连接管理命令
    
    /**
     * @brief 认证密码
     * @param password 密码
     * @return 认证成功返回true，失败返回false
     */
    bool auth(const std::string& password);
    
    /**
     * @brief 选择数据库
     * @param database 数据库索引
     * @return 选择成功返回true，失败返回false
     */
    bool select(int database);
    
    /**
     * @brief  Ping服务器
     * @return 服务器响应正常返回true，否则返回false
     */
    bool ping();
    
    // 字符串操作
    
    /**
     * @brief 设置字符串值
     * @param key 键
     * @param value 值
     * @param expire 过期时间(秒)，0表示不过期
     * @return 设置成功返回true，失败返回false
     */
    bool set(const std::string& key, const std::string& value, int expire = 0);
    
    /**
     * @brief 获取字符串值
     * @param key 键
     * @return 键对应的值，如果键不存在返回空字符串
     */
    std::string get(const std::string& key);
    
    /**
     * @brief 删除键
     * @param key 键
     * @return 删除的键数量
     */
    int64_t del(const std::string& key);
    
    /**
     * @brief 检查键是否存在
     * @param key 键
     * @return 键存在返回true，否则返回false
     */
    bool exists(const std::string& key);
    
    /**
     * @brief 设置键的过期时间
     * @param key 键
     * @param expire 过期时间(秒)
     * @return 设置成功返回true，失败返回false
     */
    bool expire(const std::string& key, int expire);

    /**
     * @brief 设置字符串值并指定过期时间（原子操作）
     * @param key 键
     * @param expire 过期时间(秒)
     * @param value 值
     * @return 设置成功返回true，失败返回false
     */
    bool setex(const std::string& key, int expire, const std::string& value);

    /**
     * @brief 设置字符串值（如果键不存在）
     * @param key 键
     * @param value 值
     * @return 设置成功返回true，键已存在返回false
     */
    bool setnx(const std::string& key, const std::string& value);

    /**
     * @brief 设置字符串值并指定过期时间（如果键不存在）
     * @param key 键
     * @param value 值
     * @param expire 过期时间(秒)
     * @return 设置成功返回true，键已存在返回false
     */
    bool setnxex(const std::string& key, const std::string& value, int expire);

    /**
     * @brief 获取旧值并设置新值
     * @param key 键
     * @param value 新值
     * @return 键的旧值，如果键不存在返回空字符串
     */
    std::string getset(const std::string& key, const std::string& value);

    /**
     * @brief 获取字符串值的长度
     * @param key 键
     * @return 值的长度，如果键不存在返回0
     */
    int64_t strlen(const std::string& key);

    /**
     * @brief 将值追加到字符串末尾
     * @param key 键
     * @param value 要追加的值
     * @return 追加后字符串的长度
     */
    int64_t append(const std::string& key, const std::string& value);

    // 数字操作
    /**
     * @brief 将键的值增加1
     * @param key 键
     * @return 增加后的值
     */
    int64_t incr(const std::string& key);

    /**
     * @brief 将键的值增加指定数值
     * @param key 键
     * @param increment 增量
     * @return 增加后的值
     */
    int64_t incrby(const std::string& key, int64_t increment);

    /**
     * @brief 将键的值增加指定浮点数
     * @param key 键
     * @param increment 增量
     * @return 增加后的值
     */
    double incrbyfloat(const std::string& key, double increment);

    /**
     * @brief 将键的值减少1
     * @param key 键
     * @return 减少后的值
     */
    int64_t decr(const std::string& key);

    /**
     * @brief 将键的值减少指定数值
     * @param key 键
     * @param decrement 减量
     * @return 减少后的值
     */
    int64_t decrby(const std::string& key, int64_t decrement);

    // 键操作
    /**
     * @brief 获取键的剩余生存时间
     * @param key 键
     * @return 剩余生存时间(秒)，-1表示没有设置过期时间，-2表示键不存在
     */
    int64_t ttl(const std::string& key);

    /**
     * @brief 获取键的剩余生存时间（毫秒）
     * @param key 键
     * @return 剩余生存时间(毫秒)，-1表示没有设置过期时间，-2表示键不存在
     */
    int64_t pttl(const std::string& key);

    /**
     * @brief 移除键的过期时间
     * @param key 键
     * @return 移除成功返回true，失败返回false
     */
    bool persist(const std::string& key);

    /**
     * @brief 重命名键
     * @param key 原键名
     * @param newkey 新键名
     * @return 重命名成功返回true，失败返回false
     */
    bool rename(const std::string& key, const std::string& newkey);

    /**
     * @brief 获取所有匹配模式的键
     * @param pattern 模式
     * @return 匹配的键列表
     */
    std::vector<std::string> keys(const std::string& pattern);
    
    // 哈希表操作
    
    /**
     * @brief 设置哈希字段值
     * @param key 哈希键
     * @param field 字段名
     * @param value 字段值
     * @return 设置成功返回true，失败返回false
     */
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    /**
     * @brief 获取哈希字段值
     * @param key 哈希键
     * @param field 字段名
     * @return 字段值，如果字段不存在返回空字符串
     */
    std::string hget(const std::string& key, const std::string& field);
    
    /**
     * @brief 获取哈希表所有字段和值
     * @param key 哈希键
     * @return 字段-值映射表
     */
    std::map<std::string, std::string> hgetall(const std::string& key);
    
    /**
     * @brief 删除哈希字段
     * @param key 哈希键
     * @param field 字段名
     * @return 删除的字段数量
     */
    int64_t hdel(const std::string& key, const std::string& field);
    
    // 列表操作
    
    /**
     * @brief 向左推入列表元素
     * @param key 列表键
     * @param value 元素值
     * @return 推入后列表长度
     */
    int64_t lpush(const std::string& key, const std::string& value);
    
    /**
     * @brief 向右推入列表元素
     * @param key 列表键
     * @param value 元素值
     * @return 推入后列表长度
     */
    int64_t rpush(const std::string& key, const std::string& value);
    
    /**
     * @brief 从左弹出列表元素
     * @param key 列表键
     * @return 弹出的元素值，如果列表为空返回空字符串
     */
    std::string lpop(const std::string& key);
    
    /**
     * @brief 从右弹出列表元素
     * @param key 列表键
     * @return 弹出的元素值，如果列表为空返回空字符串
     */
    std::string rpop(const std::string& key);
    
    /**
     * @brief 获取列表长度
     * @param key 列表键
     * @return 列表长度
     */
    int64_t llen(const std::string& key);
    
    // 集合操作
    
    /**
     * @brief 添加集合元素
     * @param key 集合键
     * @param member 成员
     * @return 添加的元素数量
     */
    int64_t sadd(const std::string& key, const std::string& member);
    
    /**
     * @brief 移除集合元素
     * @param key 集合键
     * @param member 成员
     * @return 移除的元素数量
     */
    int64_t srem(const std::string& key, const std::string& member);
    
    /**
     * @brief 检查元素是否在集合中
     * @param key 集合键
     * @param member 成员
     * @return 存在返回true，否则返回false
     */
    bool sismember(const std::string& key, const std::string& member);
    
    // 有序集合操作
    
    /**
     * @brief 添加有序集合元素
     * @param key 有序集合键
     * @param score 分数
     * @param member 成员
     * @return 添加的元素数量
     */
    int64_t zadd(const std::string& key, double score, const std::string& member);
    
    /**
     * @brief 获取有序集合成员分数
     * @param key 有序集合键
     * @param member 成员
     * @return 成员分数，如果成员不存在返回0.0
     */
    double zscore(const std::string& key, const std::string& member);
    
    // 事务操作
    
    /**
     * @brief 开始事务
     * @return 开始成功返回true，失败返回false
     */
    bool multi();
    
    /**
     * @brief 执行事务
     * @return 事务执行结果数组，执行失败返回空数组
     */
    std::vector<RedisReply::ptr> exec();
    
    /**
     * @brief 取消事务
     * @return 取消成功返回true，失败返回false
     */
    bool discard();
    
    // 发布订阅
    
    /**
     * @brief 发布消息
     * @param channel 频道
     * @param message 消息
     * @return 接收到消息的订阅者数量
     */
    int64_t publish(const std::string& channel, const std::string& message);
    
    /**
     * @brief 订阅频道
     * @param channel 频道
     * @return 订阅成功返回true，失败返回false
     */
    bool subscribe(const std::string& channel);
    
    /**
     * @brief 取消订阅
     * @param channel 频道
     * @return 取消订阅成功返回true，失败返回false
     */
    bool unsubscribe(const std::string& channel);
    
private:
    redisContext* m_context;     ///< Redis连接上下文
    std::string m_host;          ///< 主机地址
    uint16_t m_port;             ///< 端口号
    std::string m_password;      ///< 密码
    uint32_t m_timeout;          ///< 超时时间
    bool m_inTransaction;        ///< 是否在事务中
};

/**
 * @brief Redis连接池
 * 管理Redis连接池，提供连接复用和资源管理
 */
class RedisConnectionPool {
public:
    typedef std::shared_ptr<RedisConnectionPool> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param host Redis服务器主机
     * @param port Redis服务器端口
     * @param password 密码
     * @param database 数据库索引
     * @param maxConnections 最大连接数
     * @param minConnections 最小连接数
     * @param connectionTimeout 连接超时时间(秒)
     * @param idleTimeout 空闲连接超时时间(秒)
     */
    RedisConnectionPool(const std::string& host, uint16_t port,
                       const std::string& password = "", int database = 0,
                       uint32_t maxConnections = 10,
                       uint32_t minConnections = 2,
                       uint32_t connectionTimeout = 30,
                       uint32_t idleTimeout = 60);
    
    /**
     * @brief 析构函数，自动关闭所有连接
     */
    ~RedisConnectionPool();
    
    /**
     * @brief 从连接池获取连接
     * @return Redis连接指针，获取失败返回nullptr
     */
    RedisConnection::ptr getConnection();
    
    /**
     * @brief 归还连接到连接池
     * @param conn Redis连接指针
     */
    void returnConnection(RedisConnection::ptr conn);
    
    /**
     * @brief 获取活跃连接数
     * @return 当前活跃连接数量
     */
    uint32_t getActiveCount() const;
    
    /**
     * @brief 获取空闲连接数
     * @return 当前空闲连接数量
     */
    uint32_t getIdleCount() const;
    
    /**
     * @brief 获取总连接数
     * @return 当前总连接数量
     */
    uint32_t getTotalCount() const;
    
    /**
     * @brief 关闭所有连接
     */
    void closeAll();
    
private:
    /**
     * @brief 创建新连接
     * @return 创建成功返回true，失败返回false
     */
    bool createConnection();
    
    /**
     * @brief 清理空闲连接
     */
    void cleanupIdleConnections();
    
private:
    std::string m_host;                         ///< 主机地址
    uint16_t m_port;                            ///< 端口号
    std::string m_password;                     ///< 密码
    int m_database;                             ///< 数据库索引
    uint32_t m_maxConnections;                  ///< 最大连接数
    uint32_t m_minConnections;                  ///< 最小连接数
    uint32_t m_connectionTimeout;               ///< 连接超时时间
    uint32_t m_idleTimeout;                     ///< 空闲连接超时时间
    
    std::deque<RedisConnection::ptr> m_idleConnections;    ///< 空闲连接列表
    std::deque<RedisConnection::ptr> m_activeConnections;  ///< 活跃连接列表
    mutable sylar::RWMutex m_mutex;                     ///< 读写锁，保证线程安全
    
    bool m_running;                             ///< 连接池运行状态
    sylar::Timer::ptr m_cleanupTimer;           ///< 清理定时器
};

/**
 * @brief Redis管理器
 * 统一管理多个连接池，提供全局访问接口
 */
class RedisManager {
public:
    typedef std::shared_ptr<RedisManager> ptr;  ///< 智能指针类型定义

    /**
     * @brief 初始化Redis管理器
     */
    static bool Init(const std::string& conf_path = "");
    
    /**
     * @brief 添加连接池
     * @param name 连接池名称
     * @param pool 连接池指针
     * @return 添加成功返回true，失败返回false
     */
    bool addPool(const std::string& name, RedisConnectionPool::ptr pool);
    
    /**
     * @brief 获取连接池
     * @param name 连接池名称
     * @return 连接池指针，不存在返回nullptr
     */
    RedisConnectionPool::ptr getPool(const std::string& name);
    
    /**
     * @brief 从指定连接池获取连接
     * @param poolName 连接池名称
     * @return Redis连接指针，获取失败返回nullptr
     */
    RedisConnection::ptr getConnection(const std::string& poolName = "pool_1");
    
private:
    std::map<std::string, RedisConnectionPool::ptr> m_pools;  ///< 连接池映射表
    sylar::RWMutex m_mutex;                                  ///< 读写锁，保证线程安全
};

/**
 * @brief Redis管理器单例
 * 提供全局唯一的Redis管理器实例
 */
typedef sylar::Singleton<RedisManager> RedisManagerSingleton;

} // namespace db
} // namespace sylar

#endif // SYLAR_REDIS_CONNECTOR_H