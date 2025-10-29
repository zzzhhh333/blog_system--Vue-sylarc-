#ifndef BLOG_SERVER_MODEL_BLOG_H
#define BLOG_SERVER_MODEL_BLOG_H

#include <string>
#include <ctime>
#include "jsoncpp/json/json.h"

namespace blog_server {
namespace model {

/**
 * @brief 博客数据模型类
 * 
 * 表示博客文章的数据结构，包含博客的所有属性和JSON序列化功能
 */
class Blog {
public:
    /**
     * @brief 博客状态枚举
     */
    enum Status {
        STATUS_DRAFT = 0,     ///< 草稿状态
        STATUS_PUBLISHED = 1, ///< 已发布状态
        STATUS_DELETED = 2    ///< 已删除状态
    };

public:
    /**
     * @brief 默认构造函数
     */
    Blog() 
        : id(0), 
          author_id(0), 
          status(STATUS_DRAFT), 
          view_count(0), 
          created_at(0), 
          updated_at(0), 
          published_at(0) {
    }

    /**
     * @brief 转换为JSON对象
     * @return JSON值对象
     */
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

    /**
     * @brief 从JSON对象创建博客实例
     * @param json JSON值对象
     * @return 博客对象
     */
    static Blog fromJson(const Json::Value& json) {
        Blog blog;
        if (json.isMember("id")) blog.setId(json["id"].asInt64());
        if (json.isMember("title")) blog.setTitle(json["title"].asString());
        if (json.isMember("content")) blog.setContent(json["content"].asString());
        if (json.isMember("summary")) blog.setSummary(json["summary"].asString());
        if (json.isMember("author_id")) blog.setAuthorId(json["author_id"].asInt64());
        if (json.isMember("author_name")) blog.setAuthorName(json["author_name"].asString());
        if (json.isMember("status")) blog.setStatus(json["status"].asInt());
        if (json.isMember("view_count")) blog.setViewCount(json["view_count"].asInt());
        if (json.isMember("created_at")) blog.setCreatedAt(json["created_at"].asInt64());
        if (json.isMember("updated_at")) blog.setUpdatedAt(json["updated_at"].asInt64());
        if (json.isMember("published_at")) blog.setPublishedAt(json["published_at"].asInt64());
        
        // 如果没有设置时间，设置默认值
        time_t now = time(nullptr);
        if (blog.getCreatedAt() == 0) {
            blog.setCreatedAt(now);
        }
        if (blog.getUpdatedAt() == 0) {
            blog.setUpdatedAt(now);
        }
        
        // 如果状态是已发布且未设置发布时间，设置发布时间
        if (blog.getStatus() == STATUS_PUBLISHED && blog.getPublishedAt() == 0) {
            blog.setPublishedAt(now);
        }
        
        return blog;
    }

    // ==================== Getter 方法 ====================

    /**
     * @brief 获取博客ID
     * @return 博客ID
     */
    int64_t getId() const { return id; }

    /**
     * @brief 获取博客标题
     * @return 博客标题
     */
    const std::string& getTitle() const { return title; }

    /**
     * @brief 获取博客内容
     * @return 博客内容
     */
    const std::string& getContent() const { return content; }

    /**
     * @brief 获取博客摘要
     * @return 博客摘要
     */
    const std::string& getSummary() const { return summary; }

    /**
     * @brief 获取作者ID
     * @return 作者ID
     */
    int64_t getAuthorId() const { return author_id; }

    /**
     * @brief 获取作者名称
     * @return 作者名称
     */
    const std::string& getAuthorName() const { return author_name; }

    /**
     * @brief 获取博客状态
     * @return 博客状态
     */
    int getStatus() const { return status; }

    /**
     * @brief 获取浏览次数
     * @return 浏览次数
     */
    int getViewCount() const { return view_count; }

    /**
     * @brief 获取创建时间
     * @return 创建时间戳
     */
    time_t getCreatedAt() const { return created_at; }

    /**
     * @brief 获取更新时间
     * @return 更新时间戳
     */
    time_t getUpdatedAt() const { return updated_at; }

    /**
     * @brief 获取发布时间
     * @return 发布时间戳
     */
    time_t getPublishedAt() const { return published_at; }

    // ==================== Setter 方法 ====================

    /**
     * @brief 设置博客ID
     * @param value 博客ID
     */
    void setId(int64_t value) { id = value; }

    /**
     * @brief 设置博客标题
     * @param value 博客标题
     */
    void setTitle(const std::string& value) { title = value; }

    /**
     * @brief 设置博客内容
     * @param value 博客内容
     */
    void setContent(const std::string& value) { content = value; }

    /**
     * @brief 设置博客摘要
     * @param value 博客摘要
     */
    void setSummary(const std::string& value) { summary = value; }

    /**
     * @brief 设置作者ID
     * @param value 作者ID
     */
    void setAuthorId(int64_t value) { author_id = value; }

    /**
     * @brief 设置作者名称
     * @param value 作者名称
     */
    void setAuthorName(const std::string& value) { author_name = value; }

    /**
     * @brief 设置博客状态
     * @param value 博客状态
     */
    void setStatus(int value) { status = value; }

    /**
     * @brief 设置浏览次数
     * @param value 浏览次数
     */
    void setViewCount(int value) { view_count = value; }

    /**
     * @brief 设置创建时间
     * @param value 创建时间戳
     */
    void setCreatedAt(time_t value) { created_at = value; }

    /**
     * @brief 设置更新时间
     * @param value 更新时间戳
     */
    void setUpdatedAt(time_t value) { updated_at = value; }

    /**
     * @brief 设置发布时间
     * @param value 发布时间戳
     */
    void setPublishedAt(time_t value) { published_at = value; }

    // ==================== 业务方法 ====================

    /**
     * @brief 检查博客是否为草稿状态
     * @return 如果是草稿状态返回true，否则返回false
     */
    bool isDraft() const { return status == STATUS_DRAFT; }

    /**
     * @brief 检查博客是否已发布
     * @return 如果已发布返回true，否则返回false
     */
    bool isPublished() const { return status == STATUS_PUBLISHED; }

    /**
     * @brief 检查博客是否已删除
     * @return 如果已删除返回true，否则返回false
     */
    bool isDeleted() const { return status == STATUS_DELETED; }

    /**
     * @brief 增加浏览次数
     * @param count 增加的次数，默认为1
     */
    void increaseViewCount(int count = 1) { view_count += count; }

    /**
     * @brief 更新时间为当前时间
     */
    void updateTimestamp() { updated_at = time(nullptr); }

    /**
     * @brief 发布博客
     */
    void publish() {
        status = STATUS_PUBLISHED;
        time_t now = time(nullptr);
        published_at = now;
        updated_at = now;
    }

    /**
     * @brief 将博客标记为草稿
     */
    void markAsDraft() {
        status = STATUS_DRAFT;
        updated_at = time(nullptr);
    }

    /**
     * @brief 软删除博客
     */
    void softDelete() {
        status = STATUS_DELETED;
        updated_at = time(nullptr);
    }

    /**
     * @brief 检查博客是否有效（非删除状态）
     * @return 如果有效返回true，否则返回false
     */
    bool isValid() const { return status != STATUS_DELETED; }

private:
    int64_t id;                ///< 博客ID
    std::string title;         ///< 博客标题
    std::string content;       ///< 博客内容
    std::string summary;       ///< 博客摘要
    int64_t author_id;         ///< 作者ID
    std::string author_name;   ///< 作者名称
    int status;                ///< 博客状态：0-草稿, 1-已发布, 2-已删除
    int view_count;            ///< 浏览次数
    time_t created_at;         ///< 创建时间
    time_t updated_at;         ///< 更新时间
    time_t published_at;       ///< 发布时间
};

} // namespace model
} // namespace blog_server

#endif // BLOG_SERVER_MODEL_BLOG_H