#ifndef BLOG_SERVER_MODEL_RESPONSE_H
#define BLOG_SERVER_MODEL_RESPONSE_H

#include "jsoncpp/json/json.h"

namespace blog_server {
namespace model {

struct ApiResponse {
    int code;
    std::string message;
    Json::Value data;
    
    ApiResponse(int c = 0, const std::string& msg = "success", const Json::Value& d = Json::Value())
        : code(c), message(msg), data(d) {}
        
    Json::Value toJson() const {
        Json::Value json;
        json["code"] = code;
        json["message"] = message;
        json["data"] = data;
        return json;
    }
};

} // namespace model
} // namespace blog_server

#endif