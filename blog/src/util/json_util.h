#ifndef BLOG_SERVER_UTIL_JSON_UTIL_H
#define BLOG_SERVER_UTIL_JSON_UTIL_H

#include <string>
#include "jsoncpp/json/json.h"

namespace blog_server {
namespace util {

class JsonUtil {
public:
    static bool parse(const std::string& str, Json::Value& json);
    static std::string toString(const Json::Value& json);
};

} // namespace util
} // namespace blog_server

#endif