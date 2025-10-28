// blog_server/dao/user_dao.h
#ifndef BLOG_SERVER_DAO_USER_DAO_H
#define BLOG_SERVER_DAO_USER_DAO_H

#include "base_dao.h"
#include "../model/user.h"
#include "sylar/db/mysql.h"  
#include "sylar/log.h"

namespace blog_server {
namespace dao {

/**
 * @brief 用户数据访问对象类
 * 
 * 负责所有与用户表相关的数据库操作，包括：
 * - 用户信息的增删改查
 * - 用户认证相关操作
 * - 用户状态管理
 * 
 * 使用 MySQLWrapper 进行数据库操作，提供类型安全的接口。
 */
class UserDao : public BaseDao<model::User> {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化用户 DAO，设置日志记录器。
     * 需要在构造后调用 setDb() 设置数据库连接。
     */
    UserDao();
    
    /**
     * @brief 析构函数
     */
    ~UserDao() = default;

    // ==================== 基础 CRUD 操作 ====================
    
    /**
     * @brief 创建新用户
     * @param user 用户对象，成功创建后会填充 ID 和时间戳
     * @return 创建成功返回 true，失败返回 false
     * 
     * 插入新用户记录到数据库，自动设置创建时间和更新时间。
     * 密码应该在调用此方法前已经加密。
     */
    bool create(model::User& user) override;
    
    /**
     * @brief 根据ID查找用户
     * @param id 用户ID
     * @return 找到的用户对象，未找到返回空对象（id=0）
     * 
     * 只查找状态正常的用户（status=1）。
     */
    model::User findById(int64_t id) override;
    
    /**
     * @brief 更新用户信息
     * @param user 包含更新信息的用户对象
     * @return 更新成功返回 true，失败返回 false
     * 
     * 更新除密码外的用户信息，自动更新更新时间字段。
     * 不会更新创建时间和密码字段。
     */
    bool update(const model::User& user) override;
    
    /**
     * @brief 删除用户（软删除）
     * @param id 要删除的用户ID
     * @return 删除成功返回 true，失败返回 false
     * 
     * 实际执行的是更新用户状态为禁用（status=0），
     * 保留用户数据用于审计和关联查询。
     */
    bool remove(int64_t id) override;
    
    // ==================== 查询操作 ====================
    
    /**
     * @brief 查找所有用户
     * @return 用户对象列表
     * 
     * 返回所有状态正常的用户，按创建时间倒序排列。
     * 对于大量用户，建议使用分页查询。
     */
    std::vector<model::User> findAll() override;
    
    /**
     * @brief 根据条件查询用户
     * @param condition SQL WHERE 子句的条件部分
     * @return 符合条件的用户列表
     * 
     * 示例：findByCondition("status = 1 AND created_at > 1234567890")
     * 注意：条件中不需要包含 WHERE 关键字。
     */
    std::vector<model::User> findByCondition(const std::string& condition) override;
    
    /**
     * @brief 分页查询用户
     * @param page 页码（从1开始）
     * @param page_size 每页大小
     * @param order_by 排序字段，默认为按ID倒序
     * @return 指定页的用户列表
     * 
     * 用于用户列表的分页显示，只返回状态正常的用户。
     */
    std::vector<model::User> findPage(int page, int page_size, const std::string& order_by = "") override;
    
    // ==================== 统计操作 ====================
    
    /**
     * @brief 统计用户总数
     * @return 用户总数
     * 
     * 只统计状态正常的用户。
     */
    int64_t count() override;
    
    /**
     * @brief 根据条件统计用户数量
     * @param condition SQL WHERE 子句的条件部分
     * @return 符合条件的用户数量
     * 
     * 示例：countByCondition("status = 1 AND created_at > 1234567890")
     */
    int64_t countByCondition(const std::string& condition) override;
    
    // ==================== 用户特定操作 ====================
    
    /**
     * @brief 根据用户名查找用户
     * @param username 用户名
     * @return 找到的用户对象，未找到返回空对象（id=0）
     * 
     * 用于用户登录和用户名验证，只查找状态正常的用户。
     */
    model::User findByUsername(const std::string& username);
    
    /**
     * @brief 根据邮箱查找用户
     * @param email 邮箱地址
     * @return 找到的用户对象，未找到返回空对象（id=0）
     * 
     * 用于邮箱验证和密码重置，只查找状态正常的用户。
     */
    model::User findByEmail(const std::string& email);
    
    /**
     * @brief 更新用户密码
     * @param user_id 用户ID
     * @param new_password 新密码（应该已经加密）
     * @return 更新成功返回 true，失败返回 false
     * 
     * 专门用于密码更新，会自动更新更新时间字段。
     */
    bool updatePassword(int64_t user_id, const std::string& new_password);
    
    /**
     * @brief 更新用户状态
     * @param user_id 用户ID
     * @param status 新状态（0-禁用，1-正常）
     * @return 更新成功返回 true，失败返回 false
     * 
     * 用于启用/禁用用户账户，会自动更新更新时间字段。
     */
    bool updateStatus(int64_t user_id, int status);
    
    /**
     * @brief 检查用户名是否存在
     * @param username 要检查的用户名
     * @return 存在返回 true，不存在返回 false
     * 
     * 用于用户注册时的用户名重复检查。
     * 检查所有用户，包括已禁用的用户。
     */
    bool existsUsername(const std::string& username);
    
    /**
     * @brief 检查邮箱是否存在
     * @param email 要检查的邮箱地址
     * @return 存在返回 true，不存在返回 false
     * 
     * 用于用户注册时的邮箱重复检查。
     * 检查所有用户，包括已禁用的用户。
     */
    bool existsEmail(const std::string& email);

private:
    /**
     * @brief 将查询结果转换为用户对象
     * @param result MySQLWrapper 查询结果
     * @return 转换后的用户对象
     * 
     * 内部工具方法，用于将数据库查询结果转换为业务层使用的用户对象。
     * 处理字段映射和类型转换。
     */
    model::User resultToUser(sylar::MySQLWrapper::Result::ptr result);
    
private:
    sylar::Logger::ptr logger_;  ///< 日志记录器
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_USER_DAO_H