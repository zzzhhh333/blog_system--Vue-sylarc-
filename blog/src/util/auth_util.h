#ifndef BLOG_SERVER_UTIL_AUTH_UTIL_H
#define BLOG_SERVER_UTIL_AUTH_UTIL_H

#include "sylar/http/http.h"
#include "../model/user.h"

namespace blog_server {
namespace util {

class AuthUtil {
public:
    // 从请求中获取当前用户
    static model::User getCurrentUser(sylar::http::HttpRequest::ptr request);
    
    // 生成token
    static std::string generateToken(const model::User& user);
    
    // 验证token
    static bool verifyToken(const std::string& token, model::User& user);
    static model::User verifyToken(const std::string& token);
};

} // namespace util
} // namespace blog_server

#endif