// blog_server/util/jwt_util.h
#ifndef BLOG_SERVER_UTIL_JWT_UTIL_H
#define BLOG_SERVER_UTIL_JWT_UTIL_H

#include <string>
#include <memory>
#include <vector>
#include <random>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "sylar/config.h"
#include "sylar/util.h"

namespace blog_server {
namespace util {

struct JWTClaims {
    int64_t user_id;
    std::string username;
    std::vector<std::string> roles;
    std::string jti;
    int64_t iat; // 签发时间
    int64_t exp; // 过期时间
    std::string issuer;
    
    JWTClaims() : user_id(0), iat(0), exp(0) {}
    
    bool isValid() const {
        return user_id > 0 && !username.empty() && exp > 0;
    }
    
    bool isExpired() const {
        return time(nullptr) > exp;
    }
};

class JWTUtil {
public:
    /**
     * @brief 初始化JWT配置
     */
    static bool init();
    
    /**
     * @brief 生成JWT令牌
     * @param user_id 用户ID
     * @param username 用户名
     * @param roles 用户角色列表
     * @param expires_in 过期时间(秒)
     * @return JWT令牌字符串
     */
    static std::string generateToken(int64_t user_id, 
                                   const std::string& username, 
                                   const std::vector<std::string>& roles = {},
                                   int64_t expires_in = 3600 * 24);
    
    /**
     * @brief 验证JWT令牌
     * @param token JWT令牌
     * @param claims 输出参数，解码后的声明
     * @return 验证成功返回true
     */
    static bool verifyToken(const std::string& token, JWTClaims& claims);
    
    /**
     * @brief 验证JWT令牌
     * @param token JWT令牌
     * @return 验证成功返回true
     */
    static bool verifyToken(const std::string& token);
    
    /**
     * @brief 从令牌中获取用户ID
     * @param token JWT令牌
     * @return 用户ID，失败返回0
     */
    static int64_t getUserIdFromToken(const std::string& token);
    
    /**
     * @brief 从令牌中获取用户名
     * @param token JWT令牌
     * @return 用户名，失败返回空字符串
     */
    static std::string getUsernameFromToken(const std::string& token);
    
    /**
     * @brief 从令牌中获取用户角色
     * @param token JWT令牌
     * @return 用户角色列表
     */
    static std::vector<std::string> getRolesFromToken(const std::string& token);
    
    /**
     * @brief 从令牌中获取JTI
     * @param token JWT令牌
     * @return JTI，失败返回空字符串
     */
    static std::string getJTIFromToken(const std::string& token);
    
    /**
     * @brief 获取令牌过期时间
     * @param token JWT令牌
     * @return 过期时间的时间戳，失败返回0
     */
    static time_t getExpirationFromToken(const std::string& token);
    
    /**
     * @brief 刷新JWT令牌
     * @param token 原令牌
     * @param expires_in 新的过期时间
     * @return 新的JWT令牌，失败返回空字符串
     */
    static std::string refreshToken(const std::string& token, int64_t expires_in = 3600 * 24);
    
    /**
     * @brief 将令牌加入黑名单
     * @param token JWT令牌
     * @return 是否成功
     */
    static bool addToBlacklist(const std::string& token);
    
    /**
     * @brief 检查令牌是否在黑名单中
     * @param token JWT令牌
     * @return 是否在黑名单中
     */
    static bool isInBlacklist(const std::string& token);
    
    /**
     * @brief 清理过期的黑名单令牌
     */
    static void cleanupExpiredBlacklist();
    
    /**
     * @brief 获取JWT密钥
     */
    static std::string getCurrentSecret();

private:
    // Base64 URL 编码
    static std::string base64UrlEncode(const std::string& data);
    static std::string base64UrlDecode(const std::string& data);
    
    // HMAC SHA256 签名
    static std::string hmacSha256(const std::string& key, const std::string& data);
    
    // JSON 序列化和反序列化
    static std::string serializeClaims(const JWTClaims& claims);
    static bool parseClaims(const std::string& json, JWTClaims& claims);
    
    // 令牌解析和验证
    static bool parseToken(const std::string& token, JWTClaims& claims, std::string& signature);
    static bool validateTokenStructure(const JWTClaims& claims);
    static std::string generateJTI();
    
    // 安全地解码令牌
    static std::optional<JWTClaims> safeDecode(const std::string& token);

    // 添加随机字符串生成方法
    static std::string generateRandomString(size_t length);
    
    // 黑名单管理
    static bool addToRedisBlacklist(const std::string& jti, time_t expire_time);
    static bool isInRedisBlacklist(const std::string& jti);
    static bool addToMySQLBlacklist(const std::string& jti, const std::string& token, 
                                   int64_t user_id, time_t expire_time);
    static bool isInMySQLBlacklist(const std::string& jti);

private:
    static std::string current_secret_;
    static std::string previous_secret_;
    static time_t secret_rotation_time_;
};

} // namespace util
} // namespace blog_server

#endif