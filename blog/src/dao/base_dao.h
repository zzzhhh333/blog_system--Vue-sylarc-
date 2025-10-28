// blog_server/dao/base_dao.h
#ifndef BLOG_SERVER_DAO_BASE_DAO_H
#define BLOG_SERVER_DAO_BASE_DAO_H

#include "sylar/db/mysql.h"
#include <memory>
#include <string>

namespace blog_server {
namespace dao {

/**
 * @brief 基础数据访问对象类
 * 
 * 提供所有 DAO 类的公共接口和基础功能
 * 包含数据库连接管理和基本的 CRUD 操作接口
 */
template<typename T>
class BaseDao {
public:
    using ptr = std::shared_ptr<BaseDao<T>>;
    
    /**
     * @brief 构造函数
     */
    BaseDao() = default;
    
    /**
     * @brief 虚析构函数
     */
    virtual ~BaseDao() = default;
    
    /**
     * @brief 设置数据库连接
     * @param db MySQL 数据库连接指针
     */
    void setDb(sylar::MySQLWrapper::ptr db) { 
        db_ = db; 
    }
    
    /**
     * @brief 获取数据库连接
     * @return MySQL 数据库连接指针
     */
    sylar::MySQLWrapper::ptr getDb() const { 
        return db_; 
    }
    
    // ==================== 基础 CRUD 操作接口 ====================
    
    /**
     * @brief 创建实体
     * @param entity 实体对象
     * @return 创建成功返回 true，失败返回 false
     */
    virtual bool create(T& entity) = 0;
    
    /**
     * @brief 根据ID查找实体
     * @param id 实体ID
     * @return 找到的实体对象
     */
    virtual T findById(int64_t id) = 0;
    
    /**
     * @brief 更新实体
     * @param entity 实体对象
     * @return 更新成功返回 true，失败返回 false
     */
    virtual bool update(const T& entity) = 0;
    
    /**
     * @brief 删除实体
     * @param id 实体ID
     * @return 删除成功返回 true，失败返回 false
     */
    virtual bool remove(int64_t id) = 0;
    
    // ==================== 查询操作接口 ====================
    
    /**
     * @brief 查找所有实体
     * @return 实体列表
     */
    virtual std::vector<T> findAll() = 0;
    
    /**
     * @brief 根据条件查询实体
     * @param condition 查询条件
     * @return 符合条件的实体列表
     */
    virtual std::vector<T> findByCondition(const std::string& condition) = 0;
    
    /**
     * @brief 分页查询实体
     * @param page 页码
     * @param page_size 每页大小
     * @param order_by 排序字段
     * @return 分页后的实体列表
     */
    virtual std::vector<T> findPage(int page, int page_size, const std::string& order_by = "") = 0;
    
    // ==================== 统计操作接口 ====================
    
    /**
     * @brief 统计实体总数
     * @return 实体总数
     */
    virtual int64_t count() = 0;
    
    /**
     * @brief 根据条件统计实体数量
     * @param condition 统计条件
     * @return 符合条件的实体数量
     */
    virtual int64_t countByCondition(const std::string& condition) = 0;

protected:
    sylar::MySQLWrapper::ptr db_;  ///< 数据库连接
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_BASE_DAO_H