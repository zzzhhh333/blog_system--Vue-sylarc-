// mysql_wrapper.h
#ifndef SYLAR_MYSQL_WRAPPER_H
#define SYLAR_MYSQL_WRAPPER_H

#include <mysql/mysql.h>  // MySQL C API 头文件
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace sylar {

/**
 * @brief MySQL 数据库封装类
 * 
 * 这个类封装了 MySQL C API，提供了更友好、更安全的 C++ 接口。
 * 支持连接管理、SQL执行、预处理语句、事务处理等功能。
 * 使用 RAII 模式管理资源，确保资源正确释放。
 */
class MySQLWrapper {
public:
    using ptr = std::shared_ptr<MySQLWrapper>;  ///< 智能指针类型定义
    
    /**
     * @brief 数据库连接配置结构体
     * 
     * 包含连接到 MySQL 数据库所需的所有配置参数
     */
    struct Config {
        std::string host = "127.0.0.1";      ///< 数据库主机地址
        std::string user = "root";           ///< 数据库用户名
        std::string password;                ///< 数据库密码
        std::string database;                ///< 数据库名称
        unsigned int port = 3306;            ///< 数据库端口号
        std::string charset = "utf8mb4";     ///< 字符集编码
        int timeout = 30;                    ///< 连接超时时间（秒）
        int reconnect = 1;                   ///< 是否自动重连（1-是，0-否）
    };
    
    /**
     * @brief 查询结果集封装类
     * 
     * 封装了 MySQL 查询结果集，提供类型安全的数据访问方法
     */
    class Result {
    public:
        using ptr = std::shared_ptr<Result>;  ///< 智能指针类型定义

        /**
         * @brief 构造函数
         * @param res MySQL 原生结果集指针
         */
        Result(MYSQL_RES* res);
        
        /**
         * @brief 析构函数，自动释放结果集资源
         */
        ~Result();
        
        
        /**
         * @brief 移动到结果集的下一行
         * @return 如果还有下一行数据返回 true，否则返回 false
         */
        bool next();
        
        // 数据获取方法 - 按列名
        int getInt(const std::string& column);           ///< 获取整型数据（按列名）
        int64_t getInt64(const std::string& column);     ///< 获取64位整型数据（按列名）
        std::string getString(const std::string& column);///< 获取字符串数据（按列名）
        double getDouble(const std::string& column);     ///< 获取浮点数据（按列名）
        bool isNull(const std::string& column);          ///< 检查列值是否为NULL（按列名）
        
        // 数据获取方法 - 按列索引
        int getInt(int index);               ///< 获取整型数据（按列索引）
        int64_t getInt64(int index);         ///< 获取64位整型数据（按列索引）
        std::string getString(int index);    ///< 获取字符串数据（按列索引）
        double getDouble(int index);         ///< 获取浮点数据（按列索引）
        bool isNull(int index);              ///< 检查列值是否为NULL（按列索引）
        
        // 结果集信息获取方法
        int getRowCount() const { return row_count_; }       ///< 获取结果集行数
        int getFieldCount() const { return field_count_; }   ///< 获取结果集列数
        std::string getFieldName(int index) const;           ///< 获取指定列的名称
        
    private:
        
        /**
         * @brief 根据列名获取列索引
         * @param column 列名
         * @return 列索引，如果不存在返回 -1
         */
        int getColumnIndex(const std::string& column);
        
    private:
        MYSQL_RES* result_ = nullptr;                    ///< MySQL 原生结果集指针
        MYSQL_ROW row_ = nullptr;                        ///< 当前数据行
        unsigned long* lengths_ = nullptr;               ///< 当前行各列数据长度
        MYSQL_FIELD* fields_ = nullptr;                  ///< 结果集字段信息
        int field_count_ = 0;                            ///< 字段数量
        int row_count_ = 0;                              ///< 行数量
        std::unordered_map<std::string, int> field_index_map_;  ///< 字段名到索引的映射表
    };
    
    /**
     * @brief 预处理语句封装类
     * 
     * 封装了 MySQL 预处理语句，提供类型安全的参数绑定和数据访问
     * 可以有效防止 SQL 注入攻击，提高执行效率
     */
    class PreparedStatement {
    public:
        using ptr = std::shared_ptr<PreparedStatement>;  ///< 智能指针类型定义
        
        /**
         * @brief 构造函数
         * @param db 数据库连接指针
         * @param sql SQL 语句
         */
        PreparedStatement(MySQLWrapper* db, const std::string& sql);


        /**
         * @brief 析构函数，自动释放预处理语句资源
         */
        ~PreparedStatement();
        
        /**
         * @brief 执行预处理语句（用于 INSERT/UPDATE/DELETE 等）
         * @return 执行成功返回 true，失败返回 false
         */
        bool execute();
        
        /**
         * @brief 执行查询预处理语句（用于 SELECT）
         * @return 结果集指针，执行失败返回 nullptr
         */
        Result::ptr query();
        
        // 参数绑定方法
        void setInt(int paramIndex, int value);                     ///< 绑定整型参数
        void setInt64(int paramIndex, int64_t value);              ///< 绑定64位整型参数
        void setDouble(int paramIndex, double value);              ///< 绑定浮点参数
        void setString(int paramIndex, const std::string& value);  ///< 绑定字符串参数
        void setNull(int paramIndex);                              ///< 绑定NULL参数
        void setBlob(int paramIndex, const void* data, unsigned long length);  ///< 绑定二进制数据参数
        
        // 执行结果信息
        int getAffectedRows();      ///< 获取受影响的行数
        int64_t getLastInsertId();  ///< 获取最后插入的ID
        
    
        
    private:
        MySQLWrapper* db_ = nullptr;    ///< 数据库连接指针
        MYSQL_STMT* stmt_ = nullptr;    ///< MySQL 原生预处理语句指针
        MYSQL_BIND* params_ = nullptr;  ///< 参数绑定数组
        int param_count_ = 0;           ///< 参数数量
        std::vector<char*> buffers_;    ///< 参数值缓冲区（用于自动内存管理）
        std::vector<unsigned long> buffer_lengths_;  ///< 参数值长度数组
    };

public:
    /**
     * @brief 默认构造函数
     * 
     * 初始化 MySQL 连接句柄，设置默认连接选项
     */
    MySQLWrapper();
    
    /**
     * @brief 析构函数
     * 
     * 自动断开连接并释放所有资源
     */
    ~MySQLWrapper();
    
    // ==================== 连接管理方法 ====================
    
    /**
     * @brief 使用配置结构体连接到数据库
     * @param config 数据库连接配置
     * @return 连接成功返回 true，失败返回 false
     */
    bool connect(const Config& config);
    
    /**
     * @brief 使用参数连接到数据库
     * @param host 数据库主机地址
     * @param user 数据库用户名
     * @param password 数据库密码
     * @param database 数据库名称
     * @param port 数据库端口号，默认3306
     * @param charset 字符集编码，默认utf8mb4
     * @return 连接成功返回 true，失败返回 false
     */
    bool connect(const std::string& host, const std::string& user,
                const std::string& password, const std::string& database,
                unsigned int port = 3306, const std::string& charset = "utf8mb4");
    
    /**
     * @brief 断开数据库连接
     * 
     * 关闭与数据库的连接，释放相关资源
     */
    void disconnect();
    
    /**
     * @brief 检查连接状态
     * @return 连接正常返回 true，断开返回 false
     */
    bool isConnected();
    
    /**
     * @brief 检查连接并尝试重连
     * @return 连接正常或重连成功返回 true，否则返回 false
     */
    bool ping();
    
    // ==================== SQL 执行方法 ====================
    
    /**
     * @brief 执行 SQL 语句（用于 INSERT/UPDATE/DELETE 等）
     * @param sql 要执行的 SQL 语句
     * @return 执行成功返回 true，失败返回 false
     */
    bool execute(const std::string& sql);
    
    /**
     * @brief 执行查询 SQL 语句（用于 SELECT）
     * @param sql 要执行的查询 SQL 语句
     * @return 结果集指针，执行失败返回 nullptr
     */
    Result::ptr query(const std::string& sql);
    
    /**
     * @brief 创建预处理语句
     * @param sql 包含占位符的 SQL 语句
     * @return 预处理语句指针，创建失败返回 nullptr
     */
    PreparedStatement::ptr prepare(const std::string& sql);
    
    // ==================== 事务管理方法 ====================
    
    /**
     * @brief 开始事务
     * @return 开始成功返回 true，失败返回 false
     */
    bool beginTransaction();
    
    /**
     * @brief 提交事务
     * @return 提交成功返回 true，失败返回 false
     */
    bool commit();
    
    /**
     * @brief 回滚事务
     * @return 回滚成功返回 true，失败返回 false
     */
    bool rollback();
    
    // ==================== 信息获取方法 ====================
    
    /**
     * @brief 获取受影响的行数
     * @return 受影响的行数
     */
    int getAffectedRows();
    
    /**
     * @brief 获取最后插入的ID
     * @return 最后插入的自动增长ID
     */
    int64_t getLastInsertId();
    
    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    std::string getError();
    
    /**
     * @brief 获取错误代码
     * @return MySQL 错误代码
     */
    int getErrorCode();
    
    // ==================== 连接信息获取方法 ====================
    
    /**
     * @brief 获取数据库主机地址
     * @return 主机地址字符串
     */
    std::string getHost() const { return config_.host; }
    
    /**
     * @brief 获取数据库名称
     * @return 数据库名称字符串
     */
    std::string getDatabase() const { return config_.database; }

private:
    /**
     * @brief 清理错误状态
     * 
     * 重置错误状态，为下一次操作做准备
     */
    void clearError();
    
    /**
     * @brief 重新连接数据库
     * @return 重连成功返回 true，失败返回 false
     */
    bool reconnect();
    
private:
    MYSQL* mysql_ = nullptr;        ///< MySQL 连接句柄
    Config config_;                 ///< 连接配置信息
    bool in_transaction_ = false;   ///< 是否在事务中的标志
};

} // namespace sylar

#endif // SYLAR_MYSQL_WRAPPER_H