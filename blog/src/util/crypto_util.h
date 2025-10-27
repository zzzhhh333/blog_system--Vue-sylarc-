#ifndef BLOG_SERVER_UTIL_CRYPTO_UTIL_H
#define BLOG_SERVER_UTIL_CRYPTO_UTIL_H

#include <string>

namespace blog_server {
namespace util {

class CryptoUtil {
public:
    static std::string md5(const std::string& str);
    static std::string sha1(const std::string& str);
    static std::string base64Encode(const std::string& str);
    static std::string base64Decode(const std::string& str);
};

} // namespace util
} // namespace blog_server

#endif