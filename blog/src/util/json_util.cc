#include "json_util.h"
#include <sstream>
#include <iostream>

namespace blog_server {
namespace util {

bool JsonUtil::parse(const std::string& str, Json::Value& json) {
    if (str.empty()) {
        return false;
    }
    
    Json::CharReaderBuilder reader;
    std::stringstream ss(str);
    std::string errs;

    std::cout << ss.str() << std::endl; // Debug: 输出要解析的字符串
    
    bool success = Json::parseFromStream(reader, ss, &json, &errs);
    if (!success) {
        return false;
    }

    
    return true;
}

std::string JsonUtil::toString(const Json::Value& json) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 紧凑格式
    return Json::writeString(writer, json);
}

} // namespace util
} // namespace blog_server