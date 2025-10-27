#ifndef BLOG_SERVER_MODEL_USER_H
#define BLOG_SERVER_MODEL_USER_H

#include <string>
#include <ctime>
#include "jsoncpp/json/json.h"

namespace blog_server {
namespace model {

struct User {
    int64_t id;
    std::string username;
    std::string password;
    std::string nickname;
    std::string email;
    std::string avatar;
    std::string bio;
    int status; // 0:禁用, 1:正常
    time_t created_at;
    time_t updated_at;

    Json::Value toJson(bool includeSensitive = false) const {
        Json::Value json;
        json["id"] = (Json::Int64)id;
        json["username"] = username;
        if (includeSensitive) {
            json["password"] = password;
        }
        json["nickname"] = nickname;
        json["email"] = email;
        json["avatar"] = avatar;
        json["bio"] = bio;
        json["status"] = status;
        json["created_at"] = (Json::Int64)created_at;
        json["updated_at"] = (Json::Int64)updated_at;
        return json;
    }

    static User fromJson(const Json::Value& json) {
        User user;
        if (json.isMember("id")) user.id = json["id"].asInt64();
        if (json.isMember("username")) user.username = json["username"].asString();
        if (json.isMember("password")) user.password = json["password"].asString();
        if (json.isMember("nickname")) user.nickname = json["nickname"].asString();
        if (json.isMember("email")) user.email = json["email"].asString();
        if (json.isMember("avatar")) user.avatar = json["avatar"].asString();
        if (json.isMember("bio")) user.bio = json["bio"].asString();
        if (json.isMember("status")) user.status = json["status"].asInt();
        
        time_t now = time(nullptr);
        user.created_at = now;
        user.updated_at = now;
        
        return user;
    }
};

} // namespace model
} // namespace blog_server

#endif