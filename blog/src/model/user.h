#ifndef BLOG_SERVER_MODEL_USER_H
#define BLOG_SERVER_MODEL_USER_H

#include <string>
#include <ctime>
#include "jsoncpp/json/json.h"

namespace blog_server {
namespace model {

/**
 * @brief 用户数据模型类
 * 
 * 表示用户的数据结构，包含用户的所有属性和JSON序列化功能
 */
class User {
public:
    /**
     * @brief 用户状态枚举
     */
    enum Status {
        STATUS_INACTIVE = 0,  ///< 禁用状态
        STATUS_ACTIVE = 1     ///< 正常状态
    };

public:
    /**
     * @brief 默认构造函数
     */
    User() 
        : id_(0), 
          status_(STATUS_ACTIVE), 
          created_at_(0), 
          updated_at_(0),
          last_login_at_(0) {
    }

    /**
     * @brief 转换为JSON对象
     * @param includeSensitive 是否包含敏感信息（如密码）
     * @return JSON值对象
     */
    Json::Value toJson(bool includeSensitive = false) const {
        Json::Value json;
        json["id"] = (Json::Int64)id_;
        json["username"] = username_;
        if (includeSensitive) {
            json["password"] = password_;
        }
        json["nickname"] = nickname_;
        json["email"] = email_;
        json["avatar"] = avatar_;
        json["bio"] = bio_;
        json["status"] = status_;
        json["created_at"] = (Json::Int64)created_at_;
        json["updated_at"] = (Json::Int64)updated_at_;
        if (last_login_at_ > 0) {
            json["last_login_at"] = (Json::Int64)last_login_at_;
        }
        return json;
    }

    /**
     * @brief 从JSON对象创建用户实例
     * @param json JSON值对象
     * @return 用户对象
     */
    static User fromJson(const Json::Value& json) {
        User user;
        if (json.isMember("id")) user.setId(json["id"].asInt64());
        if (json.isMember("username")) user.setUsername(json["username"].asString());
        if (json.isMember("password")) user.setPassword(json["password"].asString());
        if (json.isMember("nickname")) user.setNickname(json["nickname"].asString());
        if (json.isMember("email")) user.setEmail(json["email"].asString());
        if (json.isMember("avatar")) user.setAvatar(json["avatar"].asString());
        if (json.isMember("bio")) user.setBio(json["bio"].asString());
        if (json.isMember("status")) user.setStatus(json["status"].asInt());
        if (json.isMember("created_at")) user.setCreatedAt(json["created_at"].asInt64());
        if (json.isMember("updated_at")) user.setUpdatedAt(json["updated_at"].asInt64());
        if (json.isMember("last_login_at")) user.setLastLoginAt(json["last_login_at"].asInt64());
        
        // 如果没有设置时间，设置默认值
        time_t now = time(nullptr);
        if (user.getCreatedAt() == 0) {
            user.setCreatedAt(now);
        }
        if (user.getUpdatedAt() == 0) {
            user.setUpdatedAt(now);
        }
        
        return user;
    }

    // ==================== Getter 方法 ====================

    /**
     * @brief 获取用户ID
     * @return 用户ID
     */
    int64_t getId() const { return id_; }

    /**
     * @brief 获取用户名
     * @return 用户名
     */
    const std::string& getUsername() const { return username_; }

    /**
     * @brief 获取密码
     * @return 密码
     */
    const std::string& getPassword() const { return password_; }

    /**
     * @brief 获取昵称
     * @return 昵称
     */
    const std::string& getNickname() const { return nickname_; }

    /**
     * @brief 获取邮箱
     * @return 邮箱
     */
    const std::string& getEmail() const { return email_; }

    /**
     * @brief 获取头像URL
     * @return 头像URL
     */
    const std::string& getAvatar() const { return avatar_; }

    /**
     * @brief 获取个人简介
     * @return 个人简介
     */
    const std::string& getBio() const { return bio_; }

    /**
     * @brief 获取用户状态
     * @return 用户状态
     */
    int getStatus() const { return status_; }

    /**
     * @brief 获取创建时间
     * @return 创建时间戳
     */
    time_t getCreatedAt() const { return created_at_; }

    /**
     * @brief 获取更新时间
     * @return 更新时间戳
     */
    time_t getUpdatedAt() const { return updated_at_; }

    /**
     * @brief 获取最后登录时间
     * @return 最后登录时间戳
     */
    time_t getLastLoginAt() const { return last_login_at_; }

    // ==================== Setter 方法 ====================

    /**
     * @brief 设置用户ID
     * @param value 用户ID
     */
    void setId(int64_t value) { id_ = value; }

    /**
     * @brief 设置用户名
     * @param value 用户名
     */
    void setUsername(const std::string& value) { username_ = value; }

    /**
     * @brief 设置密码
     * @param value 密码
     */
    void setPassword(const std::string& value) { password_ = value; }

    /**
     * @brief 设置昵称
     * @param value 昵称
     */
    void setNickname(const std::string& value) { nickname_ = value; }

    /**
     * @brief 设置邮箱
     * @param value 邮箱
     */
    void setEmail(const std::string& value) { email_ = value; }

    /**
     * @brief 设置头像URL
     * @param value 头像URL
     */
    void setAvatar(const std::string& value) { avatar_ = value; }

    /**
     * @brief 设置个人简介
     * @param value 个人简介
     */
    void setBio(const std::string& value) { bio_ = value; }

    /**
     * @brief 设置用户状态
     * @param value 用户状态
     */
    void setStatus(int value) { status_ = value; }

    /**
     * @brief 设置创建时间
     * @param value 创建时间戳
     */
    void setCreatedAt(time_t value) { created_at_ = value; }

    /**
     * @brief 设置更新时间
     * @param value 更新时间戳
     */
    void setUpdatedAt(time_t value) { updated_at_ = value; }

    /**
     * @brief 设置最后登录时间
     * @param value 最后登录时间戳
     */
    void setLastLoginAt(time_t value) { last_login_at_ = value; }

    // ==================== 业务方法 ====================

    /**
     * @brief 检查用户是否处于活动状态
     * @return 如果用户处于活动状态返回true，否则返回false
     */
    bool isActive() const { return status_ == STATUS_ACTIVE; }

    /**
     * @brief 检查用户是否被禁用
     * @return 如果用户被禁用返回true，否则返回false
     */
    bool isInactive() const { return status_ == STATUS_INACTIVE; }

    /**
     * @brief 激活用户
     */
    void activate() {
        status_ = STATUS_ACTIVE;
        updateTimestamp();
    }

    /**
     * @brief 禁用用户
     */
    void deactivate() {
        status_ = STATUS_INACTIVE;
        updateTimestamp();
    }

    /**
     * @brief 更新时间为当前时间
     */
    void updateTimestamp() { 
        updated_at_ = time(nullptr); 
    }

    /**
     * @brief 更新最后登录时间为当前时间
     */
    void updateLastLogin() { 
        last_login_at_ = time(nullptr); 
        updateTimestamp();
    }

    /**
     * @brief 检查用户是否有效（非禁用状态）
     * @return 如果用户有效返回true，否则返回false
     */
    bool isValid() const { return status_ != STATUS_INACTIVE; }

    /**
     * @brief 验证密码
     * @param password 要验证的密码
     * @return 密码匹配返回true，否则返回false
     */
    bool verifyPassword(const std::string& password) const {
        return password_ == password;
    }

    /**
     * @brief 清空密码
     */
    void clearPassword() {
        password_.clear();
    }

    /**
     * @brief 检查用户信息是否完整
     * @return 信息完整返回true，否则返回false
     */
    bool isComplete() const {
        return !username_.empty() && !password_.empty() && !email_.empty();
    }

    /**
     * @brief 获取显示名称
     * @return 优先返回昵称，如果昵称为空则返回用户名
     */
    std::string getDisplayName() const {
        return nickname_.empty() ? username_ : nickname_;
    }

private:
    int64_t id_;               ///< 用户ID
    std::string username_;     ///< 用户名
    std::string password_;     ///< 密码
    std::string nickname_;     ///< 昵称
    std::string email_;        ///< 邮箱
    std::string avatar_;       ///< 头像URL
    std::string bio_;          ///< 个人简介
    int status_;               ///< 用户状态：0-禁用, 1-正常
    time_t created_at_;        ///< 创建时间
    time_t updated_at_;        ///< 更新时间
    time_t last_login_at_;     ///< 最后登录时间
};

} // namespace model
} // namespace blog_server

#endif // BLOG_SERVER_MODEL_USER_H