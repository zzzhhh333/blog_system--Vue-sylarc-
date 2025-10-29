#include "user_dao.h"
#include "sylar/log.h"
#include <sstream>
#include <iomanip>

namespace blog_server {
namespace dao {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

UserDao::UserDao() 
    : BaseDao("users") {
    logger_ = g_logger;
    SYLAR_LOG_DEBUG(logger_) << "UserDao initialized";
    
    // 从连接池获取数据库连接
    if (!acquireConnection()) {
        SYLAR_LOG_ERROR(logger_) << "Failed to acquire database connection";
    }
}

UserDao::~UserDao()
{
    // 释放数据库连接
    releaseConnection();

    SYLAR_LOG_DEBUG(logger_) << "UserDao destroyed";
}

bool UserDao::create(model::User& user) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "INSERT INTO users (username, password, nickname, email, avatar, bio, status, created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare insert statement failed";
            return false;
        }
        
        // 设置时间戳
        time_t now = time(nullptr);
        user.setCreatedAt(now);
        user.setUpdatedAt(now);
        
        // 绑定参数（注意：参数索引从1开始）
        stmt->setString(1, user.getUsername());
        stmt->setString(2, user.getPassword());
        stmt->setString(3, user.getNickname());
        stmt->setString(4, user.getEmail());
        stmt->setString(5, user.getAvatar());
        stmt->setString(6, user.getBio());
        stmt->setInt32(7, user.getStatus());
        stmt->setInt64(8, user.getCreatedAt());
        stmt->setInt64(9, user.getUpdatedAt());
        
        bool result = stmt->execute();
        if (!result) {
            SYLAR_LOG_ERROR(logger_) << "Insert user failed";
            return false;
        }
        
        // 获取自增ID
        user.setId(connection_->getLastInsertId());
        
        SYLAR_LOG_INFO(logger_) << "User created successfully, ID: " << user.getId() 
                               << ", username: " << user.getUsername();
        
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Create user exception: " << e.what();
        return false;
    }
}

model::User UserDao::findById(int64_t id, bool use_cache) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return model::User();
    }
    
    try {
        // 如果有缓存实现，可以在这里添加缓存逻辑
        if (use_cache) {
            // TODO: 实现缓存逻辑
        }
        
        auto stmt = connection_->prepareStatement(
            "SELECT id, username, password, nickname, email, avatar, bio, status, "
            "created_at, updated_at, last_login_at "
            "FROM users WHERE id = ? AND status = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare select statement failed";
            return model::User();
        }
        
        stmt->setInt64(1, id);
        stmt->setInt32(2, model::User::STATUS_ACTIVE);
        
        auto result = stmt->executeQuery();
        if (!result || !result->next()) {
            SYLAR_LOG_DEBUG(logger_) << "User not found, ID: " << id;
            return model::User();
        }
        
        SYLAR_LOG_DEBUG(logger_) << "User found, ID: " << id;
        return extractEntityFromResult(result);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find user by ID exception: " << e.what();
        return model::User();
    }
}

model::User UserDao::findByUsername(const std::string& username) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return model::User();
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT id, username, password, nickname, email, avatar, bio, status, "
            "created_at, updated_at, last_login_at "
            "FROM users WHERE username = ? AND status = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare select by username statement failed";
            return model::User();
        }
        
        stmt->setString(1, username);
        stmt->setInt32(2, model::User::STATUS_ACTIVE);
        
        auto result = stmt->executeQuery();
        if (!result || !result->next()) {
            SYLAR_LOG_DEBUG(logger_) << "User not found, username: " << username;
            return model::User();
        }
        
        SYLAR_LOG_DEBUG(logger_) << "User found, username: " << username;
        return extractEntityFromResult(result);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find user by username exception: " << e.what();
        return model::User();
    }
}

model::User UserDao::findByEmail(const std::string& email) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return model::User();
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT id, username, password, nickname, email, avatar, bio, status, "
            "created_at, updated_at, last_login_at "
            "FROM users WHERE email = ? AND status = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare select by email statement failed";
            return model::User();
        }
        
        stmt->setString(1, email);
        stmt->setInt32(2, model::User::STATUS_ACTIVE);
        
        auto result = stmt->executeQuery();
        if (!result || !result->next()) {
            SYLAR_LOG_DEBUG(logger_) << "User not found, email: " << email;
            return model::User();
        }
        
        SYLAR_LOG_DEBUG(logger_) << "User found, email: " << email;
        return extractEntityFromResult(result);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find user by email exception: " << e.what();
        return model::User();
    }
}

bool UserDao::update(const model::User& user) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE users SET username = ?, nickname = ?, email = ?, avatar = ?, "
            "bio = ?, status = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        
        stmt->setString(1, user.getUsername());
        stmt->setString(2, user.getNickname());
        stmt->setString(3, user.getEmail());
        stmt->setString(4, user.getAvatar());
        stmt->setString(5, user.getBio());
        stmt->setInt32(6, user.getStatus());
        stmt->setInt64(7, now);
        stmt->setInt64(8, user.getId());
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update user failed, user ID: " << user.getId();
            return false;
        }
        
        SYLAR_LOG_INFO(logger_) << "User updated successfully, ID: " << user.getId();
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update user exception: " << e.what();
        return false;
    }
}

bool UserDao::remove(int64_t id, bool soft_delete) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        if (soft_delete) {
            // 软删除：将状态设置为已禁用
            return updateStatus(id, model::User::STATUS_INACTIVE);
        } else {
            // 硬删除：直接从数据库删除
            auto stmt = connection_->prepareStatement("DELETE FROM users WHERE id = ?");
            
            if (!stmt) {
                SYLAR_LOG_ERROR(logger_) << "Prepare hard delete statement failed";
                return false;
            }
            
            stmt->setInt64(1, id);
            int64_t affected = stmt->executeUpdate();
            
            if (affected == 0) {
                SYLAR_LOG_ERROR(logger_) << "Hard delete user failed, ID: " << id;
                return false;
            }
            
            SYLAR_LOG_INFO(logger_) << "User hard deleted successfully, ID: " << id;
            return true;
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Remove user exception: " << e.what();
        return false;
    }
}

bool UserDao::updatePassword(int64_t user_id, const std::string& new_password) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE users SET password = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update password statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setString(1, new_password);
        stmt->setInt64(2, now);
        stmt->setInt64(3, user_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update password failed, user ID: " << user_id;
            return false;
        }
        
        SYLAR_LOG_INFO(logger_) << "Password updated successfully, user ID: " << user_id;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update password exception: " << e.what();
        return false;
    }
}

bool UserDao::updateStatus(int64_t user_id, int status) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE users SET status = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update status statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setInt32(1, status);
        stmt->setInt64(2, now);
        stmt->setInt64(3, user_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update status failed, user ID: " << user_id 
                                    << ", status: " << status;
            return false;
        }
        
        std::string status_str = (status == model::User::STATUS_ACTIVE) ? "enabled" : "disabled";
        SYLAR_LOG_INFO(logger_) << "User status updated, ID: " << user_id 
                               << ", status: " << status_str;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update status exception: " << e.what();
        return false;
    }
}

std::vector<model::User> UserDao::findAll(const std::string& order_by) {
    std::string condition = "status = " + std::to_string(model::User::STATUS_ACTIVE);
    std::string actual_order_by = order_by.empty() ? "created_at DESC" : order_by;
    
    return findByCondition(condition, actual_order_by);
}

std::vector<model::User> UserDao::findByCondition(const std::string& condition, 
                                                const std::string& order_by, 
                                                int limit) {
    std::vector<model::User> users;
    
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return users;
    }
    
    try {
        std::string sql = "SELECT id, username, password, nickname, email, avatar, bio, "
                         "status, created_at, updated_at, last_login_at "
                         "FROM users";
        
        if (!condition.empty()) {
            sql += " WHERE " + condition;
        }
        
        if (!order_by.empty()) {
            sql += " ORDER BY " + order_by;
        }
        
        if (limit > 0) {
            sql += " LIMIT " + std::to_string(limit);
        }
        
        auto result = connection_->executeQuery(sql);
        users = extractEntitiesFromResult(result);
        
        SYLAR_LOG_DEBUG(logger_) << "Found " << users.size() << " users with condition: " 
                                << condition;
        return users;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find users by condition exception: " << e.what();
        return users;
    }
}

std::vector<model::User> UserDao::findPage(int page, int page_size, 
                                         const std::string& condition, 
                                         const std::string& order_by) {
    std::string actual_condition = condition.empty() ? 
        "status = " + std::to_string(model::User::STATUS_ACTIVE) : condition;
    
    std::string actual_order_by = order_by.empty() ? "created_at DESC" : order_by;
    
    std::string sql = buildPaginationSQL(page, page_size, actual_condition, actual_order_by);
    
    auto result = executeQuery(sql);
    return extractEntitiesFromResult(result);
}

int64_t UserDao::count(const std::string& condition) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return 0;
    }
    
    try {
        std::string sql = buildCountSQL(condition);
        auto result = connection_->executeQuery(sql);
        
        if (!result || !result->next()) {
            SYLAR_LOG_ERROR(logger_) << "Count users failed";
            return 0;
        }
        
        int64_t count = result->getRow().getInt64(0);
        SYLAR_LOG_DEBUG(logger_) << "User count: " << count << " with condition: " << condition;
        return count;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Count users exception: " << e.what();
        return 0;
    }
}

bool UserDao::existsUsername(const std::string& username) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT COUNT(*) FROM users WHERE username = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare exists username statement failed";
            return false;
        }
        
        stmt->setString(1, username);
        auto result = stmt->executeQuery();
        
        if (!result || !result->next()) {
            SYLAR_LOG_ERROR(logger_) << "Check username exists failed";
            return false;
        }
        
        bool exists = result->getRow().getInt64(0) > 0;
        SYLAR_LOG_DEBUG(logger_) << "Username '" << username << "' exists: " << exists;
        return exists;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Check username exists exception: " << e.what();
        return false;
    }
}

bool UserDao::existsEmail(const std::string& email) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT COUNT(*) FROM users WHERE email = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare exists email statement failed";
            return false;
        }
        
        stmt->setString(1, email);
        auto result = stmt->executeQuery();
        
        if (!result || !result->next()) {
            SYLAR_LOG_ERROR(logger_) << "Check email exists failed";
            return false;
        }
        
        bool exists = result->getRow().getInt64(0) > 0;
        SYLAR_LOG_DEBUG(logger_) << "Email '" << email << "' exists: " << exists;
        return exists;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Check email exists exception: " << e.what();
        return false;
    }
}

model::User UserDao::authenticate(const std::string& username, const std::string& password) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return model::User();
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT id, username, password, nickname, email, avatar, bio, status, "
            "created_at, updated_at, last_login_at "
            "FROM users WHERE username = ? AND password = ? AND status = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare authenticate statement failed";
            return model::User();
        }
        
        stmt->setString(1, username);
        stmt->setString(2, password);
        stmt->setInt32(3, model::User::STATUS_ACTIVE);
        
        auto result = stmt->executeQuery();
        if (!result || !result->next()) {
            SYLAR_LOG_DEBUG(logger_) << "Authentication failed for username: " << username;
            return model::User();
        }
        
        model::User user = extractEntityFromResult(result);
        
        // 更新最后登录时间
        updateLastLogin(user.getId());
        
        SYLAR_LOG_INFO(logger_) << "User authenticated successfully, username: " << username;
        return user;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Authenticate user exception: " << e.what();
        return model::User();
    }
}

bool UserDao::updateLastLogin(int64_t user_id) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE users SET last_login_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update last login statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setInt64(1, now);
        stmt->setInt64(2, user_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update last login failed, user ID: " << user_id;
            return false;
        }
        
        SYLAR_LOG_DEBUG(logger_) << "Last login time updated for user: " << user_id;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update last login exception: " << e.what();
        return false;
    }
}

bool UserDao::isEntityEmpty(const model::User& user) const {
    return user.getId() == 0;
}

model::User UserDao::extractEntityFromResult(sylar::db::MySQLResult::ptr result) {
    if (!result) {
        return model::User();
    }
    return buildUserFromRow(result->getRow());
}

model::User UserDao::buildUserFromRow(const sylar::db::MySQLRow& row) {
    model::User user;
    
    try {
        user.setId(row.getInt64("id"));
        user.setUsername(row.getString("username"));
        user.setPassword(row.getString("password"));
        user.setNickname(row.getString("nickname"));
        user.setEmail(row.getString("email"));
        user.setAvatar(row.getString("avatar"));
        user.setBio(row.getString("bio"));
        user.setStatus(row.getInt32("status"));
        user.setCreatedAt(row.getInt64("created_at"));
        user.setUpdatedAt(row.getInt64("updated_at"));
        
        if (!row.isNull("last_login_at")) {
            user.setLastLoginAt(row.getInt64("last_login_at"));
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Build user from row failed: " << e.what();
        return model::User();
    }
    
    return user;
}

} // namespace dao
} // namespace blog_server