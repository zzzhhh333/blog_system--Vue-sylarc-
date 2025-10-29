// blog_server/service/user_service.cpp
#include "user_service.h"
#include "../util/crypto_util.h"
#include "../dao/user_dao.h"
#include <algorithm>

namespace blog_server {
namespace service {

// 定义日志记录器
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/**
 * @brief 构造函数
 */
UserService::UserService() {
    SYLAR_LOG_DEBUG(g_logger) << "UserService initialized";
    user_dao_=std::make_shared<dao::UserDao>();
}

/**
 * @brief 设置用户数据访问对象
 */
void UserService::setUserDao(std::shared_ptr<dao::UserDao> user_dao) {
    user_dao_=user_dao;
    SYLAR_LOG_DEBUG(g_logger) << "UserDAO set for UserService";
}

/**
 * @brief 用户注册
 */
model::User UserService::registerUser(const std::string& username, const std::string& password, 
                                     const std::string& nickname, const std::string& email) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return model::User();
    }
    
    // 验证输入参数
    if (username.empty() || password.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Username or password is empty";
        return model::User();
    }
    
    // 检查用户名是否已存在
    if (user_dao_->existsUsername(username)) {
        SYLAR_LOG_WARN(g_logger) << "Username already exists: " << username;
        return model::User();
    }
    
    // 检查邮箱是否已存在（如果提供了邮箱）
    if (!email.empty() && user_dao_->existsEmail(email)) {
        SYLAR_LOG_WARN(g_logger) << "Email already exists: " << email;
        return model::User();
    }
    
    // 创建用户对象
    model::User user;
    user.setUsername(username);
    user.setPassword(hashPassword(password));
    user.setNickname(nickname.empty() ? username : nickname);
    user.setEmail(email);
    user.setStatus(1); // 正常状态
    
    // 保存用户
    if (!user_dao_->create(user)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to create user: " << username;
        return model::User();
    }
    
    SYLAR_LOG_INFO(g_logger) << "User registered successfully: " << username << " (ID: " << user.getId() << ")";
    return user;
}

/**
 * @brief 用户登录
 */
model::User UserService::login(const std::string& username, const std::string& password) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return model::User();
    }
    
    if (username.empty() || password.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Username or password is empty";
        return model::User();
    }
    
    // 根据用户名查找用户
    model::User user = user_dao_->findByUsername(username);
    if (user.getId() == 0) {
        SYLAR_LOG_WARN(g_logger) << "User not found: " << username;
        return model::User();
    }
    
    // 检查用户状态
    if (user.getStatus() != 1) {
        SYLAR_LOG_WARN(g_logger) << "User is disabled: " << username;
        return model::User();
    }
    
    // 验证密码
    if (!verifyPassword(password, user.getPassword())) {
        SYLAR_LOG_WARN(g_logger) << "Password verification failed for user: " << username;
        return model::User();
    }
    
    SYLAR_LOG_INFO(g_logger) << "User logged in successfully: " << username << " (ID: " << user.getId() << ")";
    return user;
}

/**
 * @brief 根据ID获取用户信息
 */
model::User UserService::getUserById(int64_t user_id) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return model::User();
    }
    
    if (user_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid user ID: " << user_id;
        return model::User();
    }
    
    model::User user = user_dao_->findById(user_id);
    if (user.getId() == 0) {
        SYLAR_LOG_DEBUG(g_logger) << "User not found by ID: " << user_id;
    }
    
    return user;
}

/**
 * @brief 根据用户名获取用户信息
 */
model::User UserService::getUserByUsername(const std::string& username) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return model::User();
    }
    
    if (username.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Username is empty";
        return model::User();
    }
    
    model::User user = user_dao_->findByUsername(username);
    if (user.getId() == 0) {
        SYLAR_LOG_DEBUG(g_logger) << "User not found by username: " << username;
    }
    
    return user;
}

/**
 * @brief 更新用户信息
 */
bool UserService::updateUser(const model::User& user) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return false;
    }

    int id=user.getId();
    
    if (id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid user ID: " << id;
        return false;
    }
    
    // 获取现有用户信息
    model::User existing_user = user_dao_->findById(id);
    if (existing_user.getId() == 0) {
        SYLAR_LOG_ERROR(g_logger) << "User not found for update: " << id;
        return false;
    }
    
    // 创建更新对象，只更新允许修改的字段
    model::User update_user = existing_user;
    update_user.setNickname(user.getNickname());
    update_user.setEmail(user.getEmail());
    update_user.setAvatar(user.getAvatar());
    update_user.setBio(user.getBio());
    
    // 执行更新
    if (!user_dao_->update(update_user)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to update user: " << id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "User updated successfully: " << id;
    return true;
}

/**
 * @brief 验证用户密码
 */
bool UserService::validateUser(int64_t user_id, const std::string& password) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return false;
    }


    
    if (user_id <= 0 || password.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid user ID or password";
        return false;
    }
    
    model::User user = user_dao_->findById(user_id);
    if (user.getId() == 0) {
        SYLAR_LOG_ERROR(g_logger) << "User not found for validation: " << user_id;
        return false;
    }
    
    bool valid = verifyPassword(password, user.getPassword());
    SYLAR_LOG_DEBUG(g_logger) << "Password validation for user " << user_id << ": " << (valid ? "valid" : "invalid");
    return valid;
}

/**
 * @brief 更新用户密码
 */
bool UserService::updatePassword(int64_t user_id, const std::string& old_password, const std::string& new_password) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return false;
    }
    
    if (user_id <= 0 || old_password.empty() || new_password.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid parameters for password update";
        return false;
    }
    
    // 验证旧密码
    if (!validateUser(user_id, old_password)) {
        SYLAR_LOG_ERROR(g_logger) << "Old password verification failed for user: " << user_id;
        return false;
    }
    
    // 加密新密码并更新
    std::string hashed_new_password = hashPassword(new_password);
    if (!user_dao_->updatePassword(user_id, hashed_new_password)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to update password for user: " << user_id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Password updated successfully for user: " << user_id;
    return true;
}

/**
 * @brief 重置用户密码
 */
bool UserService::resetPassword(int64_t user_id, const std::string& new_password) {
    if (!user_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "UserDAO not set";
        return false;
    }
    
    if (user_id <= 0 || new_password.empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid parameters for password reset";
        return false;
    }
    
    // 加密新密码并更新
    std::string hashed_new_password = hashPassword(new_password);
    if (!user_dao_->updatePassword(user_id, hashed_new_password)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to reset password for user: " << user_id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Password reset successfully for user: " << user_id;
    return true;
}




/**
 * @brief 对密码进行加密
 */
std::string UserService::hashPassword(const std::string& password) {
    // 使用 CryptoUtil 进行密码加密
    // 实际项目中应该使用更安全的加密方式，如 bcrypt
    return util::CryptoUtil::md5("blog_salt_" + password);
}

/**
 * @brief 验证密码
 */
bool UserService::verifyPassword(const std::string& password, const std::string& hashed) {
    return hashPassword(password) == hashed;
}

} // namespace service
} // namespace blog_server