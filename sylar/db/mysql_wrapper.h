// mysql_wrapper.h
#ifndef MYSQL_WRAPPER_H
#define MYSQL_WRAPPER_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace sylar {

/**
 * @brief MySQL数据库封装类
 * 
 * 这个类封装了MySQL C API，提供了简单易用的接口来操作MySQL数据库。
 * 支持连接管理、SQL执行、事务处理、预处理语句等功能。
 */
class MySQLWrapper {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化MySQL连接句柄，但不建立实际连接。
     */
    MySQLWrapper();
    
    /**
     * @brief 析构函数
     * 
     * 自动断开连接并清理资源。
     */
    ~MySQLWrapper();
    
    // ==================== 连接管理 ====================
    
    /**
     * @brief 连接到MySQL数据库
     * 
     * @param host 数据库服务器地址
     * @param user 用户名
     * @param password 密码
     * @param database 数据库名
     * @param port 端口号，默认3306
     * @param charset 字符集，默认utf8
     * @return true 连接成功
     * @return false 连接失败
     */
    bool connect(const std::string& host, const std::string& user, 
                const std::string& password, const std::string& database,
                unsigned int port = 3306, const std::string& charset = "utf8");
    
    /**
     * @brief 断开数据库连接
     * 
     * 关闭与MySQL服务器的连接，并清理结果集等资源。
     */
    void disconnect();
    
    /**
     * @brief 检查连接状态
     * 
     * @return true 连接正常
     * @return false 连接已断开
     */
    bool isConnected();
    
    // ==================== 基础操作 ====================
    
    /**
     * @brief 执行SQL语句（用于INSERT/UPDATE/DELETE等）
     * 
     * @param sql 要执行的SQL语句
     * @return true 执行成功
     * @return false 执行失败
     */
    bool execute(const std::string& sql);
    
    /**
     * @brief 执行查询语句（用于SELECT）
     * 
     * @param sql 查询SQL语句
     * @return true 查询成功
     * @return false 查询失败
     */
    bool query(const std::string& sql);
    
    /**
     * @brief 获取受影响的行数
     * 
     * @return int 受影响的行数
     */
    int getAffectedRows();
    
    /**
     * @brief 获取最后插入的ID
     * 
     * @return long long 最后插入的自增ID
     */
    long long getLastInsertId();
    
    // ==================== 结果集操作 ====================
    
    /**
     * @brief 移动到结果集的下一行
     * 
     * @return true 成功移动到下一行
     * @return false 没有更多行或出错
     */
    bool next();
    
    /**
     * @brief 根据列名获取整数值
     * 
     * @param column 列名
     * @return int 整数值
     */
    int getInt(const std::string& column);
    
    /**
     * @brief 根据列索引获取整数值
     * 
     * @param index 列索引（从0开始）
     * @return int 整数值
     */
    int getInt(int index);
    
    /**
     * @brief 根据列名获取字符串值
     * 
     * @param column 列名
     * @return std::string 字符串值
     */
    std::string getString(const std::string& column);
    
    /**
     * @brief 根据列索引获取字符串值
     * 
     * @param index 列索引（从0开始）
     * @return std::string 字符串值
     */
    std::string getString(int index);
    
    /**
     * @brief 根据列名获取浮点数值
     * 
     * @param column 列名
     * @return double 浮点数值
     */
    double getDouble(const std::string& column);
    
    /**
     * @brief 根据列索引获取浮点数值
     * 
     * @param index 列索引（从0开始）
     * @return double 浮点数值
     */
    double getDouble(int index);
    
    /**
     * @brief 检查指定列是否为NULL
     * 
     * @param column 列名
     * @return true 值为NULL
     * @return false 值不为NULL
     */
    bool isNull(const std::string& column);
    
    /**
     * @brief 检查指定列是否为NULL
     * 
     * @param index 列索引
     * @return true 值为NULL
     * @return false 值不为NULL
     */
    bool isNull(int index);
    
    // ==================== 事务操作 ====================
    
    /**
     * @brief 开始事务
     * 
     * @return true 开始成功
     * @return false 开始失败
     */
    bool beginTransaction();
    
    /**
     * @brief 提交事务
     * 
     * @return true 提交成功
     * @return false 提交失败
     */
    bool commit();
    
    /**
     * @brief 回滚事务
     * 
     * @return true 回滚成功
     * @return false 回滚失败
     */
    bool rollback();
    
    // ==================== 预处理语句 ====================
    
    /**
     * @brief 预处理语句类
     * 
     * 用于执行带参数的SQL语句，防止SQL注入攻击。
     */
    class PreparedStatement {
    public:
        /**
         * @brief 构造函数
         * 
         * @param db MySQLWrapper指针
         * @param sql 预处理SQL语句（使用?作为参数占位符）
         */
        PreparedStatement(MySQLWrapper* db, const std::string& sql);
        
        /**
         * @brief 析构函数
         * 
         * 清理预处理语句资源。
         */
        ~PreparedStatement();
        
        /**
         * @brief 执行预处理语句（用于INSERT/UPDATE/DELETE）
         * 
         * @return true 执行成功
         * @return false 执行失败
         */
        bool execute();
        
        /**
         * @brief 执行查询预处理语句（用于SELECT）
         * 
         * @return true 查询成功
         * @return false 查询失败
         */
        bool query();
        
        // 参数绑定方法
        void setInt(int paramIndex, int value);      ///< 绑定整型参数
        void setDouble(int paramIndex, double value); ///< 绑定浮点型参数
        void setString(int paramIndex, const std::string& value); ///< 绑定字符串参数
        void setNull(int paramIndex);                ///< 绑定NULL参数
        
    private:
        MYSQL_STMT* stmt_;           ///< MySQL预处理语句句柄
        MYSQL_BIND* params_;         ///< 参数绑定数组
        std::vector<char*> stringBuffers_; ///< 字符串参数缓冲区
    };
    
    /**
     * @brief 创建预处理语句
     * 
     * @param sql 预处理SQL语句
     * @return std::shared_ptr<PreparedStatement> 预处理语句智能指针
     */
    std::shared_ptr<PreparedStatement> prepare(const std::string& sql);
    
    // ==================== 错误处理 ====================
    
    /**
     * @brief 获取错误信息
     * 
     * @return std::string 错误描述
     */
    std::string getError();
    
    /**
     * @brief 获取错误代码
     * 
     * @return int MySQL错误代码
     */
    int getErrorCode();

private:
    MYSQL* mysql_;          ///< MySQL连接句柄
    MYSQL_RES* result_;     ///< 查询结果集
    MYSQL_ROW row_;         ///< 当前行数据
    unsigned int num_fields_; ///< 结果集列数
    MYSQL_FIELD* fields_;   ///< 结果集列信息
    
    /**
     * @brief 清理结果集资源
     */
    void clearResult();
    
    /**
     * @brief 根据列名获取列索引
     * 
     * @param column 列名
     * @return int 列索引，-1表示未找到
     */
    int getColumnIndex(const std::string& column);
};

}

#endif // MYSQL_WRAPPER_H