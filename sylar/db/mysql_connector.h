#ifndef SYLAR_MYSQL_CONNECTOR_H
#define SYLAR_MYSQL_CONNECTOR_H

#include <memory>
#include <string>
#include <deque>
#include <map>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/metadata.h>
#include <cppconn/datatype.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include "sylar/singleton.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include "sylar/iomanager.h"
#include "sylar/env.h"
#include "sylar/config.h"

namespace sylar {
namespace db {

/**
 * @brief MySQL异常类
 * 封装MySQL操作过程中可能抛出的异常，提供统一的异常处理接口
 */
class MySQLException : public std::exception {
public:
    /**
     * @brief 构造函数
     * @param message 异常信息
     */
    explicit MySQLException(const std::string& message) : m_message(message) {}
    
    /**
     * @brief 获取异常信息
     * @return 异常描述字符串
     */
    const char* what() const noexcept override { return m_message.c_str(); }
    
private:
    std::string m_message;  ///< 异常信息
};

/**
 * @brief MySQL连接配置
 */
class MySQLConf {
public:
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
 * @brief MySQL查询结果行
 * 封装单行查询结果，提供类型安全的数据访问方法
 */
class MySQLRow {
public:
    typedef std::shared_ptr<MySQLRow> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param resultSet MySQL查询结果集
     */
    explicit MySQLRow(sql::ResultSet* resultSet);
    
    // 数据获取方法 - 按列名访问
    
    /**
     * @brief 获取字符串类型数据
     * @param column 列名
     * @return 字符串值，如果列为空或不存在返回空字符串
     */
    std::string getString(const std::string& column) const;
    
    /**
     * @brief 获取32位整数类型数据
     * @param column 列名
     * @return 整数值，如果列为空或不存在返回0
     */
    int32_t getInt32(const std::string& column) const;
    
    /**
     * @brief 获取64位整数类型数据
     * @param column 列名
     * @return 长整数值，如果列为空或不存在返回0
     */
    int64_t getInt64(const std::string& column) const;
    
    /**
     * @brief 获取双精度浮点数类型数据
     * @param column 列名
     * @return 双精度浮点数值，如果列为空或不存在返回0.0
     */
    double getDouble(const std::string& column) const;
    
    /**
     * @brief 获取布尔类型数据
     * @param column 列名
     * @return 布尔值，如果列为空或不存在返回false
     */
    bool getBoolean(const std::string& column) const;
    
    /**
     * @brief 检查列值是否为NULL
     * @param column 列名
     * @return 如果列为NULL返回true，否则返回false
     */
    bool isNull(const std::string& column) const;
    
    // 数据获取方法 - 按列索引访问
    
    /**
     * @brief 获取字符串类型数据
     * @param columnIndex 列索引（从0开始）
     * @return 字符串值，如果列为空或不存在返回空字符串
     */
    std::string getString(int columnIndex) const;
    
    /**
     * @brief 获取32位整数类型数据
     * @param columnIndex 列索引（从0开始）
     * @return 整数值，如果列为空或不存在返回0
     */
    int32_t getInt32(int columnIndex) const;
    
    /**
     * @brief 获取64位整数类型数据
     * @param columnIndex 列索引（从0开始）
     * @return 长整数值，如果列为空或不存在返回0
     */
    int64_t getInt64(int columnIndex) const;
    
    /**
     * @brief 获取双精度浮点数类型数据
     * @param columnIndex 列索引（从0开始）
     * @return 双精度浮点数值，如果列为空或不存在返回0.0
     */
    double getDouble(int columnIndex) const;
    
    /**
     * @brief 获取布尔类型数据
     * @param columnIndex 列索引（从0开始）
     * @return 布尔值，如果列为空或不存在返回false
     */
    bool getBoolean(int columnIndex) const;
    
    /**
     * @brief 检查列值是否为NULL
     * @param columnIndex 列索引（从0开始）
     * @return 如果列为NULL返回true，否则返回false
     */
    bool isNull(int columnIndex) const;
    
    // 元数据信息
    
    /**
     * @brief 获取结果集行数
     * @return 行数
     */
    uint64_t getRowCount() const { return m_resultSet->rowsCount(); }
    
    /**
     * @brief 获取结果集列数
     * @return 列数
     */
    uint64_t getColumnCount() const { return m_metaData->getColumnCount(); }
    
    /**
     * @brief 获取列名
     * @param columnIndex 列索引（从0开始）
     * @return 列名，如果索引越界返回空字符串
     */
    std::string getColumnName(uint64_t columnIndex) const;
    
private:
    sql::ResultSet* m_resultSet;           ///< MySQL结果集指针
    sql::ResultSetMetaData* m_metaData;    ///< 结果集元数据
};

/**
 * @brief MySQL查询结果集
 * 封装查询结果集，提供遍历和访问结果的方法
 */
class MySQLResult {
public:
    typedef std::shared_ptr<MySQLResult> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param resultSet MySQL查询结果集
     */
    explicit MySQLResult(sql::ResultSet* resultSet);
    
    /**
     * @brief 析构函数，自动释放资源
     */
    ~MySQLResult();
    
    /**
     * @brief 移动到下一行
     * @return 如果还有下一行返回true，否则返回false
     */
    bool next();
    
    /**
     * @brief 获取当前行数据
     * @return MySQLRow对象，包含当前行数据
     */
    MySQLRow getRow() const;
    
    /**
     * @brief 获取结果集行数
     * @return 行数
     */
    uint64_t getRowCount() const;
    
    /**
     * @brief 获取结果集列数
     * @return 列数
     */
    uint64_t getColumnCount() const;
    
    /**
     * @brief 获取所有列名
     * @return 列名向量
     */
    std::vector<std::string> getColumnNames() const;
    
    /**
     * @brief 检查结果集是否为空
     * @return 如果结果集为空返回true，否则返回false
     */
    bool isEmpty() const;
    
private:
    sql::ResultSet* m_resultSet;   ///< MySQL结果集指针
    bool m_ownsResultSet;          ///< 是否拥有结果集所有权（负责释放）
};

/**
 * @brief MySQL预处理语句
 * 封装预处理语句，提供参数绑定和安全执行功能
 */
class MySQLPreparedStatement {
public:
    typedef std::shared_ptr<MySQLPreparedStatement> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param stmt MySQL预处理语句指针
     */
    explicit MySQLPreparedStatement(sql::PreparedStatement* stmt);
    
    /**
     * @brief 析构函数，自动释放资源
     */
    ~MySQLPreparedStatement();
    
    // 参数绑定方法
    
    /**
     * @brief 设置NULL参数
     * @param parameterIndex 参数索引（从1开始）
     */
    void setNull(int parameterIndex);
    
    /**
     * @brief 设置布尔类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param value 布尔值
     */
    void setBoolean(int parameterIndex, bool value);
    
    /**
     * @brief 设置32位整数类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param value 整数值
     */
    void setInt32(int parameterIndex, int32_t value);
    
    /**
     * @brief 设置64位整数类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param value 长整数值
     */
    void setInt64(int parameterIndex, int64_t value);
    
    /**
     * @brief 设置双精度浮点数类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param value 双精度浮点数值
     */
    void setDouble(int parameterIndex, double value);
    
    /**
     * @brief 设置字符串类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param value 字符串值
     */
    void setString(int parameterIndex, const std::string& value);
    
    /**
     * @brief 设置日期时间类型参数
     * @param parameterIndex 参数索引（从1开始）
     * @param datetime 日期时间字符串（格式：YYYY-MM-DD HH:MM:SS）
     */
    void setDateTime(int parameterIndex, const std::string& datetime);
    
    // 执行方法
    
    /**
     * @brief 执行任意SQL语句
     * @return 如果执行成功且有结果集返回true，否则返回false
     */
    bool execute();
    
    /**
     * @brief 执行查询语句
     * @return 查询结果集指针，执行失败返回nullptr
     */
    MySQLResult::ptr executeQuery();
    
    /**
     * @brief 执行更新语句（INSERT、UPDATE、DELETE）
     * @return 受影响的行数，执行失败返回0
     */
    int64_t executeUpdate();
    
    /**
     * @brief 获取更新计数
     * @return 更新操作影响的行数
     */
    uint64_t getUpdateCount() const;
    
private:
    sql::PreparedStatement* m_stmt;  ///< MySQL预处理语句指针
};

/**
 * @brief MySQL数据库连接
 * 封装MySQL数据库连接，提供连接管理、SQL执行和事务支持
 */
class MySQLConnection {
public:
    typedef std::shared_ptr<MySQLConnection> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     */
    MySQLConnection();
    
    /**
     * @brief 析构函数，自动关闭连接
     */
    ~MySQLConnection();
    
    /**
     * @brief 连接到MySQL数据库
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param username 用户名
     * @param password 密码
     * @param database 数据库名
     * @param properties 连接属性键值对
     * @return 连接成功返回true，失败返回false
     */
    bool connect(const std::string& host, uint16_t port,
                const std::string& username, const std::string& password,
                const std::string& database, 
                const std::map<std::string, std::string>& properties = {});
    
    /**
     * @brief 关闭数据库连接
     */
    void close();
    
    /**
     * @brief 检查连接是否已关闭
     * @return 连接已关闭返回true，否则返回false
     */
    bool isClosed() const;
    
    /**
     * @brief 重新连接数据库
     * @return 重连成功返回true，失败返回false
     */
    bool reconnect();
    
    // 普通SQL语句操作
    
    /**
     * @brief 执行任意SQL语句
     * @param sql SQL语句
     * @return 执行成功返回true，失败返回false
     */
    bool execute(const std::string& sql);
    
    /**
     * @brief 执行查询语句
     * @param sql 查询SQL语句
     * @return 查询结果集指针，执行失败返回nullptr
     */
    MySQLResult::ptr executeQuery(const std::string& sql);
    
    /**
     * @brief 执行更新语句
     * @param sql 更新SQL语句（INSERT、UPDATE、DELETE）
     * @return 受影响的行数，执行失败返回0
     */
    int64_t executeUpdate(const std::string& sql);
    
    // 预处理语句操作
    
    /**
     * @brief 创建预处理语句
     * @param sql 包含占位符的SQL语句
     * @return 预处理语句指针，创建失败返回nullptr
     */
    MySQLPreparedStatement::ptr prepareStatement(const std::string& sql);
    
    // 事务操作
    
    /**
     * @brief 设置自动提交模式
     * @param autoCommit 是否自动提交
     */
    void setAutoCommit(bool autoCommit);
    
    /**
     * @brief 获取自动提交模式状态
     * @return 当前是否处于自动提交模式
     */
    bool getAutoCommit() const;
    
    /**
     * @brief 提交事务
     */
    void commit();
    
    /**
     * @brief 回滚事务
     */
    void rollback();
    
    // 连接信息
    
    /**
     * @brief 获取服务器信息
     * @return MySQL服务器版本信息
     */
    std::string getServerInfo() const;
    
    /**
     * @brief 获取客户端信息
     * @return MySQL客户端驱动信息
     */
    std::string getClientInfo() const;

    // 自增ID相关操作
    
    /**
     * @brief 获取最后插入的自增ID
     * @return 最后插入的自增ID值，获取失败返回0
     */
    uint64_t getLastInsertId() const;
    
    /**
     * @brief 执行更新并返回自增ID（针对INSERT语句）
     * @param sql INSERT语句
     * @return 插入的自增ID值，执行失败返回0
     */
    uint64_t executeUpdateAndGetId(const std::string& sql);
    
    /**
     * @brief 执行预处理语句并返回自增ID（针对INSERT语句）
     * @param stmt 预处理语句指针
     * @return 插入的自增ID值，执行失败返回0
     */
    uint64_t executeUpdateAndGetId(MySQLPreparedStatement::ptr stmt);
    
private:
    sql::Connection* m_connection;  ///< MySQL连接指针
    sql::Driver* m_driver;          ///< MySQL驱动指针
    std::string m_host;             ///< 数据库主机地址
    uint16_t m_port;                ///< 数据库端口
    std::string m_username;         ///< 用户名
    std::string m_password;         ///< 密码
    std::string m_database;         ///< 数据库名
};

/**
 * @brief MySQL连接池
 * 管理数据库连接池，提供连接复用和资源管理
 */
class MySQLConnectionPool {
public:
    typedef std::shared_ptr<MySQLConnectionPool> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 构造函数
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param username 用户名
     * @param password 密码
     * @param database 数据库名
     * @param maxConnections 最大连接数
     * @param minConnections 最小连接数
     * @param connectionTimeout 连接超时时间（秒）
     * @param idleTimeout 空闲连接超时时间（秒）
     */
    MySQLConnectionPool(const std::string& host, uint16_t port,
                       const std::string& username, const std::string& password,
                       const std::string& database,
                       uint32_t maxConnections = 10,
                       uint32_t minConnections = 2,
                       uint32_t connectionTimeout = 30,
                       uint32_t idleTimeout = 60);
    
    /**
     * @brief 析构函数，自动关闭所有连接
     */
    ~MySQLConnectionPool();
    
    /**
     * @brief 从连接池获取连接
     * @return 数据库连接指针，获取失败返回nullptr
     */
    MySQLConnection::ptr getConnection();
    
    /**
     * @brief 归还连接到连接池
     * @param conn 数据库连接指针
     */
    void returnConnection(MySQLConnection::ptr conn);
    
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
    std::string m_host;                         ///< 数据库主机地址
    uint16_t m_port;                            ///< 数据库端口
    std::string m_username;                     ///< 用户名
    std::string m_password;                     ///< 密码
    std::string m_database;                     ///< 数据库名
    uint32_t m_maxConnections;                  ///< 最大连接数
    uint32_t m_minConnections;                  ///< 最小连接数
    uint32_t m_connectionTimeout;               ///< 连接超时时间
    uint32_t m_idleTimeout;                     ///< 空闲连接超时时间
    
    std::deque<MySQLConnection::ptr> m_idleConnections;    ///< 空闲连接列表
    std::deque<MySQLConnection::ptr> m_activeConnections;  ///< 活跃连接列表
    mutable sylar::RWMutex m_mutex;                     ///< 读写锁，保证线程安全
    
    bool m_running;                             ///< 连接池运行状态
    sylar::Timer::ptr m_cleanupTimer;           ///< 清理定时器
};

/**
 * @brief 预处理语句缓存
 * 缓存预处理语句，避免重复创建，提高性能
 */
class MySQLStatementCache {
public:
    typedef std::shared_ptr<MySQLStatementCache> ptr;  ///< 智能指针类型定义
    
    /**
     * @brief 获取预处理语句
     * @param conn 数据库连接
     * @param sql SQL语句
     * @return 预处理语句指针，获取失败返回nullptr
     */
    MySQLPreparedStatement::ptr getStatement(MySQLConnection::ptr conn, 
                                           const std::string& sql);
    
    /**
     * @brief 归还预处理语句到缓存
     * @param conn 数据库连接
     * @param sql SQL语句
     * @param stmt 预处理语句指针
     */
    void returnStatement(MySQLConnection::ptr conn, 
                        const std::string& sql, 
                        MySQLPreparedStatement::ptr stmt);
    
    /**
     * @brief 清理指定连接的所有预处理语句
     * @param conn 数据库连接
     */
    void clearConnection(MySQLConnection::ptr conn);
    
    /**
     * @brief 清理所有缓存的预处理语句
     */
    void clear();
    
private:
    /**
     * @brief 缓存键结构
     */
    struct CacheKey {
        uint64_t connectionId;  ///< 连接ID
        std::string sqlMd5;     ///< SQL语句的MD5哈希值
        
        /**
         * @brief 相等比较运算符
         */
        bool operator==(const CacheKey& other) const {
            return connectionId == other.connectionId && sqlMd5 == other.sqlMd5;
        }
    };
    
    /**
     * @brief 缓存键哈希函数
     */
    struct CacheKeyHash {
        /**
         * @brief 哈希函数
         */
        std::size_t operator()(const CacheKey& key) const {
            return std::hash<uint64_t>()(key.connectionId) ^ 
                   (std::hash<std::string>()(key.sqlMd5) << 1);
        }
    };
    
    /// 缓存数据结构：连接ID -> SQL哈希 -> 预处理语句列表
    std::unordered_map<CacheKey, std::vector<MySQLPreparedStatement::ptr>, CacheKeyHash> m_cache;
    sylar::RWMutex m_mutex;  ///< 读写锁，保证线程安全
};

/**
 * @brief MySQL管理器
 * 统一管理多个连接池，提供全局访问接口
 */
class MySQLManager {
public:
    typedef std::shared_ptr<MySQLManager> ptr;  ///< 智能指针类型定义


    /**
     * @brief 初始化MySQL管理器
     */
    static bool Init(const std::string& conf_path = "");
    
    /**
     * @brief 添加连接池
     * @param name 连接池名称
     * @param pool 连接池指针
     * @return 添加成功返回true，失败返回false
     */
    bool addPool(const std::string& name, MySQLConnectionPool::ptr pool);
    
    /**
     * @brief 获取连接池
     * @param name 连接池名称
     * @return 连接池指针，不存在返回nullptr
     */
    MySQLConnectionPool::ptr getPool(const std::string& name);
    
    /**
     * @brief 从指定连接池获取连接
     * @param poolName 连接池名称
     * @return 数据库连接指针，获取失败返回nullptr
     */
    MySQLConnection::ptr getConnection(const std::string& poolName = "pool_1");
    
    /**
     * @brief 获取预处理语句缓存
     * @return 预处理语句缓存指针
     */
    MySQLStatementCache::ptr getStatementCache() { return m_stmtCache; }
    
private:
    std::map<std::string, MySQLConnectionPool::ptr> m_pools;  ///< 连接池映射表
    MySQLStatementCache::ptr m_stmtCache;                     ///< 预处理语句缓存
    sylar::RWMutex m_mutex;                                  ///< 读写锁，保证线程安全
};

/**
 * @brief MySQL管理器单例
 * 提供全局唯一的MySQL管理器实例
 */
typedef sylar::Singleton<MySQLManager> MySQLManagerSingleton;

} // namespace db
} // namespace sylar

#endif // SYLAR_MYSQL_CONNECTOR_H