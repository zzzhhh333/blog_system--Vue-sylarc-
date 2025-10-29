// blog_server/dao/base_dao.h
#ifndef BLOG_SERVER_DAO_BASE_DAO_H
#define BLOG_SERVER_DAO_BASE_DAO_H

#include "sylar/db/mysql_connector.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace blog_server {
namespace dao {

/**
 * @brief 基础数据访问对象类 (Base Data Access Object)
 * 
 * 提供所有 DAO 类的公共接口和基础功能，基于 MySQL Connector/C++ 封装
 * 包含数据库连接管理、基本的 CRUD 操作、分页查询和事务支持
 * 
 * @tparam T 实体类型
 * @tparam IdType 主键类型，默认为 int64_t
 */
template<typename T, typename IdType = int64_t>
class BaseDao {
public:
    using ptr = std::shared_ptr<BaseDao<T, IdType>>;           ///< 智能指针类型
    using EntityType = T;                                      ///< 实体类型
    using IdValueType = IdType;                                ///< 主键类型
    
    /**
     * @brief 构造函数
     * @param table_name 数据库表名
     */
    explicit BaseDao(const std::string& table_name) 
        : table_name_(table_name) {}
    
    /**
     * @brief 虚析构函数
     */
    virtual ~BaseDao() = default;
    
    // ==================== 数据库连接管理 ====================
    
    /**
     * @brief 设置数据库连接
     * @param conn MySQL 数据库连接指针
     */
    void setConnection(sylar::db::MySQLConnection::ptr conn) { 
        connection_ = conn; 
    }
    
    /**
     * @brief 获取数据库连接
     * @return MySQL 数据库连接指针
     */
    sylar::db::MySQLConnection::ptr getConnection() const { 
        return connection_; 
    }
    
    /**
     * @brief 从连接池获取数据库连接
     * @param pool_name 连接池名称，默认为 "default"
     * @return 获取成功返回 true，失败返回 false
     */
    bool acquireConnection(const std::string& pool_name = "pool_1") {
        auto mgr = sylar::db::MySQLManagerSingleton::GetInstance();
        connection_ = mgr->getConnection(pool_name);
        return connection_ != nullptr;
    }
    
    /**
     * @brief 释放数据库连接回连接池
     */
    void releaseConnection() {
        if (connection_) {
            auto mgr = sylar::db::MySQLManagerSingleton::GetInstance();
            auto pool = mgr->getPool("pool_1"); // 假设使用默认连接池
            if (pool) {
                pool->returnConnection(connection_);
            }
            connection_ = nullptr;
        }
    }
    
    // ==================== 基础 CRUD 操作接口 ====================
    
    /**
     * @brief 创建实体
     * @param entity 实体对象引用
     * @return 创建成功返回 true，失败返回 false
     */
    virtual bool create(T& entity) = 0;
    
    /**
     * @brief 批量创建实体
     * @param entities 实体对象列表
     * @return 成功创建的实体数量
     */
    virtual size_t createBatch(const std::vector<T>& entities) {
        if (!connection_ || entities.empty()) {
            return 0;
        }
        
        size_t success_count = 0;
        for (const auto& entity : entities) {
            T temp = entity; // 创建临时对象，因为create需要非const引用
            if (create(temp)) {
                success_count++;
            }
        }
        return success_count;
    }
    
    /**
     * @brief 根据主键查找实体
     * @param id 实体主键
     * @param use_cache 是否使用缓存（如果实现缓存的话）
     * @return 找到的实体对象，如果未找到返回空对象（需要实体类有默认构造函数）
     */
    virtual T findById(IdType id, bool use_cache = false) = 0;
    
    /**
     * @brief 根据主键列表批量查找实体
     * @param ids 主键列表
     * @return 实体对象列表
     */
    virtual std::vector<T> findByIds(const std::vector<IdType>& ids) {
        if (ids.empty()) {
            return {};
        }
        
        std::vector<T> result;
        for (const auto& id : ids) {
            T entity = findById(id);
            if (!isEntityEmpty(entity)) { // 假设有检查实体是否为空的方法
                result.push_back(entity);
            }
        }
        return result;
    }
    
    /**
     * @brief 更新实体
     * @param entity 实体对象
     * @return 更新成功返回 true，失败返回 false
     */
    virtual bool update(const T& entity) = 0;
    
    /**
     * @brief 批量更新实体
     * @param entities 实体对象列表
     * @return 成功更新的实体数量
     */
    virtual size_t updateBatch(const std::vector<T>& entities) {
        if (!connection_ || entities.empty()) {
            return 0;
        }
        
        size_t success_count = 0;
        for (const auto& entity : entities) {
            if (update(entity)) {
                success_count++;
            }
        }
        return success_count;
    }
    
    /**
     * @brief 删除实体
     * @param id 实体主键
     * @param soft_delete 是否软删除（如果表支持的话）
     * @return 删除成功返回 true，失败返回 false
     */
    virtual bool remove(IdType id, bool soft_delete = false) = 0;
    
    /**
     * @brief 批量删除实体
     * @param ids 主键列表
     * @param soft_delete 是否软删除
     * @return 成功删除的实体数量
     */
    virtual size_t removeBatch(const std::vector<IdType>& ids, bool soft_delete = false) {
        if (ids.empty()) {
            return 0;
        }
        
        size_t success_count = 0;
        for (const auto& id : ids) {
            if (remove(id, soft_delete)) {
                success_count++;
            }
        }
        return success_count;
    }
    
    // ==================== 查询操作接口 ====================
    
    /**
     * @brief 查找所有实体
     * @param order_by 排序字段，如 "id DESC"
     * @return 实体列表
     */
    virtual std::vector<T> findAll(const std::string& order_by = "") = 0;
    
    /**
     * @brief 根据条件查询实体
     * @param condition 查询条件，如 "status = 1"
     * @param order_by 排序字段
     * @param limit 限制返回数量，0表示不限制
     * @return 符合条件的实体列表
     */
    virtual std::vector<T> findByCondition(const std::string& condition, 
                                         const std::string& order_by = "", 
                                         int limit = 0) = 0;
    
    /**
     * @brief 分页查询实体
     * @param page 页码（从1开始）
     * @param page_size 每页大小
     * @param condition 查询条件
     * @param order_by 排序字段
     * @return 分页后的实体列表
     */
    virtual std::vector<T> findPage(int page, int page_size, 
                                  const std::string& condition = "", 
                                  const std::string& order_by = "") = 0;
    
    /**
     * @brief 根据字段值查询单个实体
     * @param field_name 字段名
     * @param field_value 字段值
     * @return 找到的实体对象
     */
    virtual T findOneByField(const std::string& field_name, const std::string& field_value) {
        auto results = findByCondition(field_name + " = '" + field_value + "'", "", 1);
        return results.empty() ? T() : results[0];
    }
    
    /**
     * @brief 根据字段值查询实体列表
     * @param field_name 字段名
     * @param field_value 字段值
     * @param order_by 排序字段
     * @return 实体列表
     */
    virtual std::vector<T> findByField(const std::string& field_name, const std::string& field_value,
                                     const std::string& order_by = "") {
        return findByCondition(field_name + " = '" + field_value + "'", order_by);
    }
    
    // ==================== 统计操作接口 ====================
    
    /**
     * @brief 统计实体总数
     * @param condition 统计条件
     * @return 实体总数
     */
    virtual int64_t count(const std::string& condition = "") = 0;
    
    /**
     * @brief 检查实体是否存在
     * @param id 实体主键
     * @return 存在返回 true，否则返回 false
     */
    virtual bool exists(IdType id) {
        return count("id = " + std::to_string(id)) > 0;
    }
    
    /**
     * @brief 根据字段值检查实体是否存在
     * @param field_name 字段名
     * @param field_value 字段值
     * @return 存在返回 true，否则返回 false
     */
    virtual bool existsByField(const std::string& field_name, const std::string& field_value) {
        return count(field_name + " = '" + field_value + "'") > 0;
    }
    
    // ==================== 事务操作接口 ====================
    
    /**
     * @brief 开始事务
     * @return 开始成功返回 true，失败返回 false
     */
    bool beginTransaction() {
        if (!connection_) return false;
        try {
            connection_->setAutoCommit(false);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief 提交事务
     * @return 提交成功返回 true，失败返回 false
     */
    bool commitTransaction() {
        if (!connection_) return false;
        try {
            connection_->commit();
            connection_->setAutoCommit(true); // 恢复自动提交
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief 回滚事务
     * @return 回滚成功返回 true，失败返回 false
     */
    bool rollbackTransaction() {
        if (!connection_) return false;
        try {
            connection_->rollback();
            connection_->setAutoCommit(true); // 恢复自动提交
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // ==================== 工具方法 ====================
    
    /**
     * @brief 获取表名
     * @return 表名字符串
     */
    const std::string& getTableName() const { 
        return table_name_; 
    }
    
    /**
     * @brief 设置表名
     * @param table_name 表名
     */
    void setTableName(const std::string& table_name) { 
        table_name_ = table_name; 
    }
    
    /**
     * @brief 检查数据库连接是否有效
     * @return 连接有效返回 true，否则返回 false
     */
    bool isConnectionValid() const {
        return connection_ && !connection_->isClosed();
    }
    
    /**
     * @brief 执行原始SQL查询
     * @param sql SQL语句
     * @return 查询结果集
     */
    sylar::db::MySQLResult::ptr executeQuery(const std::string& sql) {
        if (!isConnectionValid()) {
            return nullptr;
        }
        return connection_->executeQuery(sql);
    }
    
    /**
     * @brief 执行原始SQL更新
     * @param sql SQL语句
     * @return 受影响的行数
     */
    int64_t executeUpdate(const std::string& sql) {
        if (!isConnectionValid()) {
            return 0;
        }
        return connection_->executeUpdate(sql);
    }

protected:
    /**
     * @brief 检查实体是否为空（需要子类实现）
     * @param entity 实体对象
     * @return 实体为空返回 true，否则返回 false
     */
    virtual bool isEntityEmpty(const T& entity) const = 0;
    
    /**
     * @brief 构建分页SQL
     * @param page 页码
     * @param page_size 每页大小
     * @param condition 查询条件
     * @param order_by 排序字段
     * @return 分页SQL语句
     */
    virtual std::string buildPaginationSQL(int page, int page_size, 
                                         const std::string& condition = "", 
                                         const std::string& order_by = "") const {
        std::string sql = "SELECT * FROM " + table_name_;
        
        if (!condition.empty()) {
            sql += " WHERE " + condition;
        }
        
        if (!order_by.empty()) {
            sql += " ORDER BY " + order_by;
        }
        
        if (page > 0 && page_size > 0) {
            int offset = (page - 1) * page_size;
            sql += " LIMIT " + std::to_string(page_size) + 
                   " OFFSET " + std::to_string(offset);
        }
        
        return sql;
    }
    
    /**
     * @brief 构建计数SQL
     * @param condition 计数条件
     * @return 计数SQL语句
     */
    virtual std::string buildCountSQL(const std::string& condition = "") const {
        std::string sql = "SELECT COUNT(*) FROM " + table_name_;
        
        if (!condition.empty()) {
            sql += " WHERE " + condition;
        }
        
        return sql;
    }
    
    /**
     * @brief 从结果集提取实体（需要子类实现）
     * @param result 查询结果集
     * @return 实体对象
     */
    virtual T extractEntityFromResult(sylar::db::MySQLResult::ptr result) = 0;
    
    /**
     * @brief 从结果集提取实体列表
     * @param result 查询结果集
     * @return 实体对象列表
     */
    virtual std::vector<T> extractEntitiesFromResult(sylar::db::MySQLResult::ptr result) {
        std::vector<T> entities;
        if (!result) {
            return entities;
        }
        
        while (result->next()) {
            T entity = extractEntityFromResult(result);
            if (!isEntityEmpty(entity)) {
                entities.push_back(entity);
            }
        }
        
        return entities;
    }

protected:
    sylar::db::MySQLConnection::ptr connection_;  ///< 数据库连接
    std::string table_name_;                      ///< 数据库表名
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_BASE_DAO_H