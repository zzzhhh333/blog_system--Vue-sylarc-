#include "auth_util.h"
#include "crypto_util.h"
#include "../service/user_service.h"
#include <map>
#include <mutex>

namespace blog_server {
namespace util {


std::string AuthUtil::getCurrentToken(sylar::http::HttpRequest::ptr request) {
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
            return std::string();
        }
    } else {
        // 从Authorization头获取：Bearer <token>
        if (auth_header.find("Bearer ") == 0) {
            auth_header = auth_header.substr(7);
        }
    }
    
    if (auth_header.empty()) {
        return std::string();
    }
    
    return auth_header;
}



} // namespace util
} // namespace blog_server