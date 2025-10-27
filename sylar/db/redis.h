// sylar/db/redis.h
#ifndef SYLAR_DB_REDIS_H
#define SYLAR_DB_REDIS_H

#include "sylar/singleton.h"
#include "sylar/mutex.h"
#include "redis_wrapper.h"
#include "sylar/config.h"
#include <memory>
#include <vector>

namespace sylar {

/**
 * @brief Redis连接配置
 */
struct RedisConf {
    std::string host;           ///< 主机地址
    uint32_t port;              ///< 端口号
    std::string password;       ///< 密码
    uint32_t timeout = 0;       ///< 连接超时时间(毫秒)
    uint32_t pool_size = 5;     ///< 连接池大小
    
    /**
     * @brief 从YAML节点加载配置
     */
    bool loadFromYaml(const YAML::Node& node);
};

/**
 * @brief Redis连接池
 */
class RedisPool : public std::enable_shared_from_this<RedisPool> {
public:
    typedef std::shared_ptr<RedisPool> ptr;
    typedef RWMutex RWMutexType;
    
    /**
     * @brief 构造函数
     */
    RedisPool(const RedisConf& conf);
    
    /**
     * @brief 析构函数
     */
    ~RedisPool();
    
    /**
     * @brief 获取连接
     */
    std::shared_ptr<RedisWrapper> getConnection();
    
    /**
     * @brief 归还连接
     */
    void returnConnection(std::shared_ptr<RedisWrapper> conn);
    
    /**
     * @brief 获取配置
     */
    const RedisConf& getConf() const { return m_conf; }
    
    /**
     * @brief 检查连接池状态
     */
    void checkConnections();

private:
    RedisConf m_conf;                                   ///< 连接配置
    std::vector<std::shared_ptr<RedisWrapper>> m_conns; ///< 连接池
    RWMutexType m_mutex;                                ///< 读写锁


    /**
     * @brief 创建新的Redis连接
     * @return std::shared_ptr<RedisWrapper> 新连接，失败返回nullptr
     */
    std::shared_ptr<RedisWrapper> createNewConnection();
};

/**
 * @brief Redis管理器
 */
class RedisManager {
public:
    typedef std::shared_ptr<RedisManager> ptr;
    typedef RWMutex RWMutexType;
    
    /**
     * @brief 初始化Redis管理器
     */
    static bool Init(const std::string& conf_path = "");
    
    /**
     * @brief 获取Redis管理器实例
     */
    static RedisManager* GetInstance();
    
    /**
     * @brief 获取连接池
     */
    RedisPool::ptr getPool(const std::string& name = "master");
    
    /**
     * @brief 获取连接
     */
    std::shared_ptr<RedisWrapper> getConnection(const std::string& name = "master");
    
    /**
     * @brief 添加连接池
     */
    void addPool(const std::string& name, RedisPool::ptr pool);

private:
    RedisManager() = default;
    
private:
    std::map<std::string, RedisPool::ptr> m_pools; ///< 连接池映射
    RWMutexType m_mutex;                           ///< 读写锁
};

/// Redis管理器单例
typedef sylar::Singleton<RedisManager> RedisMgr;

} // namespace sylar

#endif // SYLAR_DB_REDIS_H