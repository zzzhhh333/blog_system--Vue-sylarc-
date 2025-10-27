#include "user_service.h"
#include "../util/crypto_util.h"
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>

namespace blog_server {
namespace service {

// 使用内存存储模拟数据库
static std::map<int64_t, model::User> s_users;
static std::map<std::string, int64_t> s_username_to_id;
static std::mutex s_user_mutex;
static int64_t s_next_user_id = 1;

UserService::UserService() {}

model::User UserService::registerUser(const std::string& username, const std::string& password, 
                                     const std::string& nickname, const std::string& email) {
    std::lock_guard<std::mutex> lock(s_user_mutex);
    
    // 检查用户名是否已存在
    if (s_username_to_id.find(username) != s_username_to_id.end()) {
        return model::User(); // 返回空对象表示注册失败
    }
    
    // 创建用户
    model::User user;
    user.id = generateUserId();
    user.username = username;
    user.password = hashPassword(password);
    user.nickname = nickname.empty() ? username : nickname;
    user.email = email;
    user.status = 1;
    user.created_at = time(nullptr);
    user.updated_at = time(nullptr);
    
    // 保存用户
    s_users[user.id] = user;
    s_username_to_id[username] = user.id;
    
    return user;
}

model::User UserService::login(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(s_user_mutex);
    
    auto it = s_username_to_id.find(username);
    if (it == s_username_to_id.end()) {
        return model::User(); // 用户不存在
    }
    
    auto user_it = s_users.find(it->second);
    if (user_it == s_users.end()) {
        return model::User();
    }
    
    // 验证密码
    if (!verifyPassword(password, user_it->second.password)) {
        return model::User();
    }
    
    return user_it->second;
}

model::User UserService::getUserById(int64_t user_id) {
    std::lock_guard<std::mutex> lock(s_user_mutex);
    
    auto it = s_users.find(user_id);
    if (it != s_users.end()) {
        return it->second;
    }
    return model::User();
}

model::User UserService::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(s_user_mutex);
    
    auto it = s_username_to_id.find(username);
    if (it != s_username_to_id.end()) {
        return getUserById(it->second);
    }
    return model::User();
}

bool UserService::updateUser(const model::User& user) {
    std::lock_guard<std::mutex> lock(s_user_mutex);
    
    auto it = s_users.find(user.id);
    if (it == s_users.end()) {
        return false;
    }
    
    // 更新用户信息
    model::User& existing_user = it->second;
    existing_user.nickname = user.nickname;
    existing_user.email = user.email;
    existing_user.avatar = user.avatar;
    existing_user.bio = user.bio;
    existing_user.updated_at = time(nullptr);
    
    return true;
}

bool UserService::validateUser(int64_t user_id, const std::string& password) {
    auto user = getUserById(user_id);
    if (user.id == 0) {
        return false;
    }
    
    return verifyPassword(password, user.password);
}

int64_t UserService::generateUserId() {
    return s_next_user_id++;
}

std::string UserService::hashPassword(const std::string& password) {
    // 实际项目中应该使用更安全的哈希算法
    return util::CryptoUtil::md5("salt_" + password);
}

bool UserService::verifyPassword(const std::string& password, const std::string& hashed) {
    return hashPassword(password) == hashed;
}

} // namespace service
} // namespace blog_server