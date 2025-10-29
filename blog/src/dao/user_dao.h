#ifndef BLOG_SERVER_DAO_USER_DAO_H
#define BLOG_SERVER_DAO_USER_DAO_H

#include "base_dao.h"
#include "../model/user.h"
#include "sylar/log.h"
#include <vector>
#include <string>

namespace blog_server {
namespace dao {

/**
 * @brief 用户数据访问对象
 * 
 * 提供用户数据的 CRUD 操作和业务相关查询方法
 */
class UserDao : public BaseDao<model::User, int64_t> {
public:
    using ptr = std::shared_ptr<UserDao>;
    
    /**
     * @brief 构造函数
     */
    UserDao();
    
    /**
     * @brief 析构函数
     */
    virtual ~UserDao();
    
    // ==================== 基础 CRUD 操作实现 ====================
    
    /**
     * @brief 创建用户
     * @param user 用户对象引用
     * @return 创建成功返回 true，失败返回 false
     */
    virtual bool create(model::User& user) override;
    
    /**
     * @brief 根据ID查找用户
     * @param id 用户ID
     * @param use_cache 是否使用缓存
     * @return 用户对象
     */
    virtual model::User findById(int64_t id, bool use_cache = false) override;
    
    /**
     * @brief 更新用户信息
     * @param user 用户对象
     * @return 更新成功返回 true，失败返回 false
     */
    virtual bool update(const model::User& user) override;
    
    /**
     * @brief 删除用户（软删除）
     * @param id 用户ID
     * @param soft_delete 是否软删除
     * @return 删除成功返回 true，失败返回 false
     */
    virtual bool remove(int64_t id, bool soft_delete = true) override;
    
    /**
     * @brief 查找所有用户
     * @param order_by 排序字段
     * @return 用户列表
     */
    virtual std::vector<model::User> findAll(const std::string& order_by = "") override;
    
    /**
     * @brief 根据条件查询用户
     * @param condition 查询条件
     * @param order_by 排序字段
     * @param limit 限制数量
     * @return 用户列表
     */
    virtual std::vector<model::User> findByCondition(const std::string& condition, 
                                                   const std::string& order_by = "", 
                                                   int limit = 0) override;
    
    /**
     * @brief 分页查询用户
     * @param page 页码
     * @param page_size 每页大小
     * @param condition 查询条件
     * @param order_by 排序字段
     * @return 用户列表
     */
    virtual std::vector<model::User> findPage(int page, int page_size, 
                                            const std::string& condition = "", 
                                            const std::string& order_by = "") override;
    
    /**
     * @brief 统计用户数量
     * @param condition 统计条件
     * @return 用户数量
     */
    virtual int64_t count(const std::string& condition = "") override;
    
    // ==================== 业务相关查询方法 ====================
    
    /**
     * @brief 根据用户名查找用户
     * @param username 用户名
     * @return 用户对象
     */
    model::User findByUsername(const std::string& username);
    
    /**
     * @brief 根据邮箱查找用户
     * @param email 邮箱地址
     * @return 用户对象
     */
    model::User findByEmail(const std::string& email);
    
    /**
     * @brief 更新用户密码
     * @param user_id 用户ID
     * @param new_password 新密码（应该已经加密）
     * @return 更新成功返回 true，失败返回 false
     */
    bool updatePassword(int64_t user_id, const std::string& new_password);
    
    /**
     * @brief 更新用户状态
     * @param user_id 用户ID
     * @param status 新状态
     * @return 更新成功返回 true，失败返回 false
     */
    bool updateStatus(int64_t user_id, int status);
    
    /**
     * @brief 检查用户名是否存在
     * @param username 要检查的用户名
     * @return 存在返回 true，不存在返回 false
     */
    bool existsUsername(const std::string& username);
    
    /**
     * @brief 检查邮箱是否存在
     * @param email 要检查的邮箱地址
     * @return 存在返回 true，不存在返回 false
     */
    bool existsEmail(const std::string& email);
    
    /**
     * @brief 验证用户密码
     * @param username 用户名
     * @param password 密码（应该已经加密）
     * @return 验证成功返回用户对象，失败返回空对象
     */
    model::User authenticate(const std::string& username, const std::string& password);
    
    /**
     * @brief 更新用户最后登录时间
     * @param user_id 用户ID
     * @return 更新成功返回 true，失败返回 false
     */
    bool updateLastLogin(int64_t user_id);

protected:
    /**
     * @brief 检查用户是否为空
     * @param user 用户对象
     * @return 为空返回 true，否则返回 false
     */
    virtual bool isEntityEmpty(const model::User& user) const override;
    
    /**
     * @brief 从结果集提取用户对象
     * @param result 查询结果集
     * @return 用户对象
     */
    virtual model::User extractEntityFromResult(sylar::db::MySQLResult::ptr result) override;
    
    /**
     * @brief 构建用户对象
     * @param row 结果行
     * @return 用户对象
     */
    model::User buildUserFromRow(const sylar::db::MySQLRow& row);

private:
    sylar::Logger::ptr logger_;  ///< 日志器
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_USER_DAO_H