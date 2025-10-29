#ifndef BLOG_SERVER_UTIL_AUTH_UTIL_H
#define BLOG_SERVER_UTIL_AUTH_UTIL_H

#include "sylar/http/http.h"
#include <string>
namespace blog_server {
namespace util {

class AuthUtil {
public:
    // 从请求中获取当前用户
    static std::string getCurrentToken(sylar::http::HttpRequest::ptr request);
};

} // namespace util
} // namespace blog_server

#endif