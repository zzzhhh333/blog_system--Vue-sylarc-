// blog_server/dao/user_dao.cpp
#include "user_dao.h"
#include "sylar/db/mysql.h"
#include <sstream>
#include <iomanip>

namespace blog_server {
namespace dao {

// 定义日志记录器
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/**
 * @brief 构造函数
 * 
 * 初始化用户 DAO，设置日志记录器。
 * 数据库连接需要在构造后通过 setDb() 方法设置。
 */
UserDao::UserDao() {
    logger_ = g_logger;
    SYLAR_LOG_DEBUG(logger_) << "UserDao initialized";
    db_=sylar::MySQLMgr::GetInstance()->getConnection();
}

/**
 * @brief 创建新用户
 * @param user 用户对象，成功创建后会填充 ID 和时间戳
 * @return 创建成功返回 true，失败返回 false
 * 
 * 插入新用户记录到数据库，自动设置创建时间和更新时间。
 * 密码应该在调用此方法前已经加密。
 */
bool UserDao::create(model::User& user) {
    // 检查数据库连接
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    // 使用预处理语句防止 SQL 注入
    auto stmt = db_->prepare(
        "INSERT INTO users (username, password, nickname, email, avatar, bio, status, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare insert statement failed";
        return false;
    }
    
    // 设置时间戳
    time_t now = time(nullptr);
    user.created_at = now;
    user.updated_at = now;
    
    // 绑定参数
    stmt->setString(0, user.username);
    stmt->setString(1, user.password);
    stmt->setString(2, user.nickname);
    stmt->setString(3, user.email);
    stmt->setString(4, user.avatar);
    stmt->setString(5, user.bio);
    stmt->setInt(6, user.status);
    stmt->setInt64(7, user.created_at);
    stmt->setInt64(8, user.updated_at);
    
    // 执行插入
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Insert user failed: " << db_->getError();
        return false;
    }
    
    // 获取自增ID
    user.id = stmt->getLastInsertId();
    SYLAR_LOG_INFO(logger_) << "User created successfully, ID: " << user.id 
                           << ", username: " << user.username;
    
    return true;
}

/**
 * @brief 根据ID查找用户
 * @param id 用户ID
 * @return 找到的用户对象，未找到返回空对象（id=0）
 * 
 * 只查找状态正常的用户（status=1）。
 */
model::User UserDao::findById(int64_t id) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return model::User();
    }
    
    // 使用预处理语句查询
    auto stmt = db_->prepare(
        "SELECT id, username, password, nickname, email, avatar, bio, status, created_at, updated_at "
        "FROM users WHERE id = ? AND status = 1"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare select statement failed";
        return model::User();
    }
    
    stmt->setInt64(1, id);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_DEBUG(logger_) << "User not found, ID: " << id;
        return model::User();
    }
    
    SYLAR_LOG_DEBUG(logger_) << "User found, ID: " << id;
    return resultToUser(result);
}

/**
 * @brief 根据用户名查找用户
 * @param username 用户名
 * @return 找到的用户对象，未找到返回空对象（id=0）
 * 
 * 用于用户登录和用户名验证，只查找状态正常的用户。
 */
model::User UserDao::findByUsername(const std::string& username) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return model::User();
    }
    
    auto stmt = db_->prepare(
        "SELECT id, username, password, nickname, email, avatar, bio, status, created_at, updated_at "
        "FROM users WHERE username = ? AND status = 1"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare select by username statement failed";
        return model::User();
    }
    
    stmt->setString(1, username);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_DEBUG(logger_) << "User not found, username: " << username;
        return model::User();
    }
    
    SYLAR_LOG_DEBUG(logger_) << "User found, username: " << username;
    return resultToUser(result);
}

/**
 * @brief 根据邮箱查找用户
 * @param email 邮箱地址
 * @return 找到的用户对象，未找到返回空对象（id=0）
 * 
 * 用于邮箱验证和密码重置，只查找状态正常的用户。
 */
model::User UserDao::findByEmail(const std::string& email) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return model::User();
    }
    
    auto stmt = db_->prepare(
        "SELECT id, username, password, nickname, email, avatar, bio, status, created_at, updated_at "
        "FROM users WHERE email = ? AND status = 1"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare select by email statement failed";
        return model::User();
    }
    
    stmt->setString(1, email);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_DEBUG(logger_) << "User not found, email: " << email;
        return model::User();
    }
    
    SYLAR_LOG_DEBUG(logger_) << "User found, email: " << email;
    return resultToUser(result);
}

/**
 * @brief 更新用户信息
 * @param user 包含更新信息的用户对象
 * @return 更新成功返回 true，失败返回 false
 * 
 * 更新除密码外的用户信息，自动更新更新时间字段。
 * 不会更新创建时间和密码字段。
 */
bool UserDao::update(const model::User& user) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare(
        "UPDATE users SET username = ?, nickname = ?, email = ?, avatar = ?, bio = ?, status = ?, updated_at = ? "
        "WHERE id = ?"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare update statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    
    stmt->setString(1, user.username);
    stmt->setString(2, user.nickname);
    stmt->setString(3, user.email);
    stmt->setString(4, user.avatar);
    stmt->setString(5, user.bio);
    stmt->setInt(6, user.status);
    stmt->setInt64(7, now);
    stmt->setInt64(8, user.id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Update user failed: " << db_->getError()
                                << ", user ID: " << user.id;
        return false;
    }
    
    SYLAR_LOG_INFO(logger_) << "User updated successfully, ID: " << user.id;
    return true;
}

/**
 * @brief 更新用户密码
 * @param user_id 用户ID
 * @param new_password 新密码（应该已经加密）
 * @return 更新成功返回 true，失败返回 false
 * 
 * 专门用于密码更新，会自动更新更新时间字段。
 */
bool UserDao::updatePassword(int64_t user_id, const std::string& new_password) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE users SET password = ?, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare update password statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setString(1, new_password);
    stmt->setInt64(2, now);
    stmt->setInt64(3, user_id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Update password failed: " << db_->getError()
                                << ", user ID: " << user_id;
        return false;
    }
    
    SYLAR_LOG_INFO(logger_) << "Password updated successfully, user ID: " << user_id;
    return true;
}

/**
 * @brief 删除用户（软删除）
 * @param id 要删除的用户ID
 * @return 删除成功返回 true，失败返回 false
 * 
 * 实际执行的是更新用户状态为禁用（status=0），
 * 保留用户数据用于审计和关联查询。
 */
bool UserDao::remove(int64_t id) {
    return updateStatus(id, 0);
}

/**
 * @brief 更新用户状态
 * @param user_id 用户ID
 * @param status 新状态（0-禁用，1-正常）
 * @return 更新成功返回 true，失败返回 false
 * 
 * 用于启用/禁用用户账户，会自动更新更新时间字段。
 */
bool UserDao::updateStatus(int64_t user_id, int status) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE users SET status = ?, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare update status statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setInt(1, status);
    stmt->setInt64(2, now);
    stmt->setInt64(3, user_id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Update status failed: " << db_->getError()
                                << ", user ID: " << user_id << ", status: " << status;
        return false;
    }
    
    std::string status_str = (status == 1) ? "enabled" : "disabled";
    SYLAR_LOG_INFO(logger_) << "User status updated, ID: " << user_id << ", status: " << status_str;
    return true;
}

/**
 * @brief 查找所有用户
 * @return 用户对象列表
 * 
 * 返回所有状态正常的用户，按创建时间倒序排列。
 * 对于大量用户，建议使用分页查询。
 */
std::vector<model::User> UserDao::findAll() {
    return findByCondition("status = 1 ORDER BY created_at DESC");
}

/**
 * @brief 根据条件查询用户
 * @param condition SQL WHERE 子句的条件部分
 * @return 符合条件的用户列表
 * 
 * 示例：findByCondition("status = 1 AND created_at > 1234567890")
 * 注意：条件中不需要包含 WHERE 关键字。
 */
std::vector<model::User> UserDao::findByCondition(const std::string& condition) {
    std::vector<model::User> users;
    
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return users;
    }
    
    std::string sql = "SELECT id, username, password, nickname, email, avatar, bio, status, created_at, updated_at "
                      "FROM users";
    if (!condition.empty()) {
        sql += " WHERE " + condition;
    }
    
    auto result = db_->query(sql);
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Query users failed: " << db_->getError();
        return users;
    }
    
    while (result->next()) {
        users.push_back(resultToUser(result));
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Found " << users.size() << " users with condition: " << condition;
    return users;
}

/**
 * @brief 分页查询用户
 * @param page 页码（从1开始）
 * @param page_size 每页大小
 * @param order_by 排序字段，默认为按ID倒序
 * @return 指定页的用户列表
 * 
 * 用于用户列表的分页显示，只返回状态正常的用户。
 */
std::vector<model::User> UserDao::findPage(int page, int page_size, const std::string& order_by) {
    std::vector<model::User> users;
    
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return users;
    }
    
    if (page < 1) page = 1;
    if (page_size < 1) page_size = 10;
    
    std::string sql = "SELECT id, username, password, nickname, email, avatar, bio, status, created_at, updated_at "
                      "FROM users WHERE status = 1";
    
    if (!order_by.empty()) {
        sql += " ORDER BY " + order_by;
    } else {
        sql += " ORDER BY id DESC";
    }
    
    sql += " LIMIT ? OFFSET ?";
    
    auto stmt = db_->prepare(sql);
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare paged query statement failed";
        return users;
    }
    
    int offset = (page - 1) * page_size;
    stmt->setInt(1, page_size);
    stmt->setInt(2, offset);
    
    auto result = stmt->query();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Execute paged query failed: " << db_->getError();
        return users;
    }
    
    while (result->next()) {
        users.push_back(resultToUser(result));
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Found " << users.size() << " users on page " << page 
                            << " (size: " << page_size << ")";
    return users;
}

/**
 * @brief 统计用户总数
 * @return 用户总数
 * 
 * 只统计状态正常的用户。
 */
int64_t UserDao::count() {
    return countByCondition("status = 1");
}

/**
 * @brief 根据条件统计用户数量
 * @param condition SQL WHERE 子句的条件部分
 * @return 符合条件的用户数量
 * 
 * 示例：countByCondition("status = 1 AND created_at > 1234567890")
 */
int64_t UserDao::countByCondition(const std::string& condition) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return 0;
    }
    
    std::string sql = "SELECT COUNT(*) FROM users";
    if (!condition.empty()) {
        sql += " WHERE " + condition;
    }
    
    auto result = db_->query(sql);
    if (!result || !result->next()) {
        SYLAR_LOG_ERROR(logger_) << "Count users failed: " << db_->getError();
        return 0;
    }
    
    int64_t count = result->getInt64(0);
    SYLAR_LOG_DEBUG(logger_) << "User count: " << count << " with condition: " << condition;
    return count;
}

/**
 * @brief 检查用户名是否存在
 * @param username 要检查的用户名
 * @return 存在返回 true，不存在返回 false
 * 
 * 用于用户注册时的用户名重复检查。
 * 检查所有用户，包括已禁用的用户。
 */
bool UserDao::existsUsername(const std::string& username) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("SELECT COUNT(*) FROM users WHERE username = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare exists username statement failed";
        return false;
    }
    
    stmt->setString(1, username);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_ERROR(logger_) << "Check username exists failed: " << db_->getError();
        return false;
    }
    
    bool exists = result->getInt64(0) > 0;
    SYLAR_LOG_DEBUG(logger_) << "Username '" << username << "' exists: " << exists;
    return exists;
}

/**
 * @brief 检查邮箱是否存在
 * @param email 要检查的邮箱地址
 * @return 存在返回 true，不存在返回 false
 * 
 * 用于用户注册时的邮箱重复检查。
 * 检查所有用户，包括已禁用的用户。
 */
bool UserDao::existsEmail(const std::string& email) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("SELECT COUNT(*) FROM users WHERE email = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare exists email statement failed";
        return false;
    }
    
    stmt->setString(1, email);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_ERROR(logger_) << "Check email exists failed: " << db_->getError();
        return false;
    }
    
    bool exists = result->getInt64(0) > 0;
    SYLAR_LOG_DEBUG(logger_) << "Email '" << email << "' exists: " << exists;
    return exists;
}

/**
 * @brief 将查询结果转换为用户对象
 * @param result MySQL 查询结果
 * @return 转换后的用户对象
 * 
 * 内部工具方法，用于将数据库查询结果转换为业务层使用的用户对象。
 * 处理字段映射和类型转换。
 */
model::User UserDao::resultToUser(sylar::MySQLWrapper::Result::ptr result) {
    model::User user;
    
    try {
        user.id = result->getInt64("id");
        user.username = result->getString("username");
        user.password = result->getString("password");
        user.nickname = result->getString("nickname");
        user.email = result->getString("email");
        user.avatar = result->getString("avatar");
        user.bio = result->getString("bio");
        user.status = result->getInt("status");
        user.created_at = result->getInt64("created_at");
        user.updated_at = result->getInt64("updated_at");
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Convert result to user failed: " << e.what();
        return model::User(); // 返回空对象
    }
    
    return user;
}



} // namespace dao
} // namespace blog_server