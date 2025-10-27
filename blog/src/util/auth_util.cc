#include "auth_util.h"
#include "crypto_util.h"
#include "../service/user_service.h"
#include <map>
#include <mutex>

namespace blog_server {
namespace util {

// 简单的token存储（实际项目应该使用Redis等）
static std::map<std::string, model::User> s_tokens;
static std::mutex s_token_mutex;

model::User AuthUtil::getCurrentUser(sylar::http::HttpRequest::ptr request) {
    std::string auth_header = request->getHeader("Authorization");
    if (auth_header.empty()) {
        // 尝试从cookie获取
        auth_header = request->getHeader("Cookie");
        if (auth_header.find("token=") != std::string::npos) {
            // 简化处理，实际应该正确解析cookie
            size_t start = auth_header.find("token=") + 6;
            size_t end = auth_header.find(";", start);
            if (end == std::string::npos) {
                auth_header = auth_header.substr(start);
            } else {
                auth_header = auth_header.substr(start, end - start);
            }
        } else {
            return model::User();
        }
    } else {
        // 从Authorization头获取：Bearer <token>
        if (auth_header.find("Bearer ") == 0) {
            auth_header = auth_header.substr(7);
        }
    }
    
    if (auth_header.empty()) {
        return model::User();
    }
    
    return verifyToken(auth_header);
}

std::string AuthUtil::generateToken(const model::User& user) {
    std::string token = CryptoUtil::md5(user.username + std::to_string(time(nullptr)) + "secret_salt");
    
    std::lock_guard<std::mutex> lock(s_token_mutex);
    s_tokens[token] = user;
    
    return token;
}

bool AuthUtil::verifyToken(const std::string& token, model::User& user) {
    std::lock_guard<std::mutex> lock(s_token_mutex);
    
    auto it = s_tokens.find(token);
    if (it != s_tokens.end()) {
        user = it->second;
        return true;
    }
    
    return false;
}

model::User AuthUtil::verifyToken(const std::string& token) {
    model::User user;
    verifyToken(token, user);
    return user;
}

} // namespace util
} // namespace blog_server