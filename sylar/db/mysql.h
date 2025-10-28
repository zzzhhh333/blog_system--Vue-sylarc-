// sylar/db/mysql.h
#ifndef SYLAR_DB_MYSQL_H
#define SYLAR_DB_MYSQL_H

#include "sylar/singleton.h"
#include "sylar/mutex.h"
#include "mysql_wrapper.h"
#include "sylar/config.h"
#include <memory>
#include <vector>

namespace sylar {

/**
 * @brief MySQL连接配置
 */
struct MySQLConf {
    std::string host;           ///< 主机地址
    uint32_t port;              ///< 端口号
    std::string user;           ///< 用户名
    std::string password;       ///< 密码
    std::string database;       ///< 数据库名
    std::string charset = "utf8"; ///< 字符集
    uint32_t timeout = 30;      ///< 连接超时时间(秒)
    
    /**
     * @brief 从YAML节点加载配置
     */
    bool loadFromYaml(const YAML::Node& node);
};

/**
 * @brief MySQL连接池
 */
class MySQLPool : public std::enable_shared_from_this<MySQLPool> {
public:
    typedef std::shared_ptr<MySQLPool> ptr;
    typedef RWMutex RWMutexType;
    
    /**
     * @brief 构造函数
     */
    MySQLPool(const MySQLConf& conf, uint32_t max_conn = 10);
    
    /**
     * @brief 析构函数
     */
    ~MySQLPool();
    
    /**
     * @brief 获取连接
     */
    std::shared_ptr<MySQLWrapper> getConnection();
    
    /**
     * @brief 归还连接
     */
    void returnConnection(std::shared_ptr<MySQLWrapper> conn);
    
    /**
     * @brief 获取配置
     */
    const MySQLConf& getConf() const { return m_conf; }
    
    /**
     * @brief 检查连接池状态
     */
    void checkConnections();

private:
    MySQLConf m_conf;                                   ///< 连接配置
    uint32_t m_maxConn;                                 ///< 最大连接数
    std::vector<std::shared_ptr<MySQLWrapper>> m_conns; ///< 连接池
    RWMutexType m_mutex;                                ///< 读写锁
};

/**
 * @brief MySQL管理器
 */
class MySQLManager {
public:
    typedef std::shared_ptr<MySQLManager> ptr;
    typedef RWMutex RWMutexType;
    MySQLManager() = default;
    /**
     * @brief 初始化MySQL管理器
     */
    static bool Init(const std::string& conf_path = "");
    
    /**
     * @brief 获取MySQL管理器实例
     */
    static MySQLManager* GetInstance();
    
    /**
     * @brief 获取连接池
     */
    MySQLPool::ptr getPool(const std::string& name = "master");
    
    /**
     * @brief 获取连接
     */
    std::shared_ptr<MySQLWrapper> getConnection(const std::string& name = "master");
    
    /**
     * @brief 添加连接池
     */
    void addPool(const std::string& name, MySQLPool::ptr pool);

private:
    
    
private:
    std::map<std::string, MySQLPool::ptr> m_pools; ///< 连接池映射
    RWMutexType m_mutex;                           ///< 读写锁
};

/// MySQL管理器单例
typedef sylar::Singleton<MySQLManager> MySQLMgr;

} // namespace sylar

#endif // SYLAR_DB_MYSQL_H