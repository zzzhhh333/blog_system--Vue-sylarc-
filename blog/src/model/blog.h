#ifndef BLOG_SERVER_MODEL_BLOG_H
#define BLOG_SERVER_MODEL_BLOG_H

#include <string>
#include <ctime>
#include "jsoncpp/json/json.h"

namespace blog_server {
namespace model {

struct Blog {
    int64_t id;
    std::string title;
    std::string content;
    std::string summary;
    int64_t author_id;
    std::string author_name;
    int status; // 0:草稿, 1:已发布, 2:已删除
    int view_count;
    time_t created_at;
    time_t updated_at;
    time_t published_at;

    Json::Value toJson() const {
        Json::Value json;
        json["id"] = (Json::Int64)id;
        json["title"] = title;
        json["content"] = content;
        json["summary"] = summary;
        json["author_id"] = (Json::Int64)author_id;
        json["author_name"] = author_name;
        json["status"] = status;
        json["view_count"] = view_count;
        json["created_at"] = (Json::Int64)created_at;
        json["updated_at"] = (Json::Int64)updated_at;
        if (published_at > 0) {
            json["published_at"] = (Json::Int64)published_at;
        }
        return json;
    }

    static Blog fromJson(const Json::Value& json) {
        Blog blog;
        if (json.isMember("id")) blog.id = json["id"].asInt64();
        if (json.isMember("title")) blog.title = json["title"].asString();
        if (json.isMember("content")) blog.content = json["content"].asString();
        if (json.isMember("summary")) blog.summary = json["summary"].asString();
        if (json.isMember("author_id")) blog.author_id = json["author_id"].asInt64();
        if (json.isMember("status")) blog.status = json["status"].asInt();
        
        time_t now = time(nullptr);
        blog.created_at = now;
        blog.updated_at = now;
        blog.view_count = 0;
        
        if (blog.status == 1) { // 已发布
            blog.published_at = now;
        }
        
        return blog;
    }
};

} // namespace model
} // namespace blog_server

#endif