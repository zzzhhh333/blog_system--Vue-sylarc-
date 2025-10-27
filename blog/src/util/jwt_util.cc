// blog_server/util/jwt_util.cpp
#include "jwt_util.h"
#include "sylar/log.h"
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/rand.h>

namespace blog_server {
namespace util {

// 静态成员变量定义
std::string JWTUtil::current_secret_;
std::string JWTUtil::previous_secret_;
time_t JWTUtil::secret_rotation_time_ = 0;

// 定义日志器
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool JWTUtil::init() {
    try {
        // 生成或从配置中获取JWT密钥
        current_secret_ = getJWTSecret();
        if (current_secret_.empty()) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to generate JWT secret";
            return false;
        }
        
        secret_rotation_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        
        SYLAR_LOG_INFO(g_logger) << "JWTUtil initialized successfully";
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "JWTUtil init failed: " << e.what();
        return false;
    }
}

std::string JWTUtil::generateToken(int64_t user_id, 
                                 const std::string& username, 
                                 const std::vector<std::string>& roles,
                                 int64_t expires_in) {
    try {
        auto now = std::chrono::system_clock::now();
        auto expire_time = now + std::chrono::seconds(expires_in);
        
        // 生成JTI（JWT ID）
        std::string jti = generateJTI();
        
        // 创建JWT builder
        auto token_builder = jwt::create()
            .set_issuer("blog_server")
            .set_subject(std::to_string(user_id))
            .set_issued_at(now)
            .set_expires_at(expire_time)
            .set_id(jti)
            .set_type("JWT")
            .set_payload_claim("username", jwt::claim(username))
            .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)));
        
        // 添加角色信息
        if (!roles.empty()) {
            std::vector<jwt::claim> role_claims;
            for (const auto& role : roles) {
                role_claims.emplace_back(jwt::claim(role));
            }
            token_builder.set_payload_claim("roles", jwt::claim(role_claims));
        }
        
        // 使用当前密钥签名
        std::string token = token_builder.sign(jwt::algorithm::hs256{current_secret_});
        
        SYLAR_LOG_DEBUG(g_logger) << "Generated JWT token for user " << username 
                                 << " (ID: " << user_id << ")";
        return token;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to generate JWT token: " << e.what();
        return "";
    }
}

bool JWTUtil::verifyToken(const std::string& token, jwt::decoded_jwt& decoded_jwt) {
    try {
        // 安全解码
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return false;
        }
        
        decoded_jwt = decoded_opt.value();
        
        // 验证令牌结构
        if (!validateTokenStructure(decoded_jwt)) {
            return false;
        }
        
        // 检查黑名单
        std::string jti = getJTIFromToken(token);
        if (!jti.empty() && isInBlacklist(jti)) {
            SYLAR_LOG_WARN(g_logger) << "Token is in blacklist, JTI: " << jti;
            return false;
        }
        
        // 使用当前密钥验证
        try {
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{current_secret_})
                .with_issuer("blog_server");
            verifier.verify(decoded_jwt);
            return true;
        } catch (const jwt::token_verification_exception&) {
            // 如果当前密钥验证失败，尝试使用上一个密钥（用于密钥轮换）
            if (!previous_secret_.empty()) {
                try {
                    auto previous_verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{previous_secret_})
                        .with_issuer("blog_server");
                    previous_verifier.verify(decoded_jwt);
                    SYLAR_LOG_DEBUG(g_logger) << "Token verified using previous secret";
                    return true;
                } catch (const jwt::token_verification_exception& e) {
                    SYLAR_LOG_WARN(g_logger) << "Token verification failed with both secrets: " << e.what();
                    return false;
                }
            }
            return false;
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Token verification error: " << e.what();
        return false;
    }
}

bool JWTUtil::verifyToken(const std::string& token) {
    jwt::decoded_jwt decoded_jwt;
    return verifyToken(token, decoded_jwt);
}

int64_t JWTUtil::getUserIdFromToken(const std::string& token) {
    try {
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return 0;
        }
        
        const auto& decoded_jwt = decoded_opt.value();
        auto claims = decoded_jwt.get_payload_claims();
        
        auto it = claims.find("user_id");
        if (it != claims.end()) {
            return std::stoll(it->second.as_string());
        }
        
        // 如果没有user_id声明，尝试从subject中获取
        return std::stoll(decoded_jwt.get_subject());
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get user ID from token: " << e.what();
        return 0;
    }
}

std::string JWTUtil::getUsernameFromToken(const std::string& token) {
    try {
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return "";
        }
        
        const auto& decoded_jwt = decoded_opt.value();
        auto claims = decoded_jwt.get_payload_claims();
        
        auto it = claims.find("username");
        if (it != claims.end()) {
            return it->second.as_string();
        }
        
        return "";
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get username from token: " << e.what();
        return "";
    }
}

std::vector<std::string> JWTUtil::getRolesFromToken(const std::string& token) {
    std::vector<std::string> roles;
    
    try {
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return roles;
        }
        
        const auto& decoded_jwt = decoded_opt.value();
        auto claims = decoded_jwt.get_payload_claims();
        
        auto it = claims.find("roles");
        if (it != claims.end()) {
            auto role_array = it->second.as_array();
            for (const auto& role_claim : role_array) {
                roles.push_back(role_claim.as_string());
            }
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get roles from token: " << e.what();
    }
    
    return roles;
}

std::string JWTUtil::getJTIFromToken(const std::string& token) {
    try {
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return "";
        }
        
        const auto& decoded_jwt = decoded_opt.value();
        return decoded_jwt.get_id();
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get JTI from token: " << e.what();
        return "";
    }
}

time_t JWTUtil::getExpirationFromToken(const std::string& token) {
    try {
        auto decoded_opt = safeDecode(token);
        if (!decoded_opt) {
            return 0;
        }
        
        const auto& decoded_jwt = decoded_opt.value();
        auto exp = decoded_jwt.get_expires_at();
        return std::chrono::system_clock::to_time_t(exp);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to get expiration from token: " << e.what();
        return 0;
    }
}

std::string JWTUtil::refreshToken(const std::string& token, int64_t expires_in) {
    try {
        // 验证原令牌
        if (!verifyToken(token)) {
            SYLAR_LOG_WARN(g_logger) << "Cannot refresh invalid token";
            return "";
        }
        
        // 获取原令牌中的用户信息
        int64_t user_id = getUserIdFromToken(token);
        std::string username = getUsernameFromToken(token);
        std::vector<std::string> roles = getRolesFromToken(token);
        
        if (user_id == 0 || username.empty()) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to extract user info from token for refresh";
            return "";
        }
        
        // 将原令牌加入黑名单
        addToBlacklist(token);
        
        // 生成新令牌
        return generateToken(user_id, username, roles, expires_in);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to refresh token: " << e.what();
        return "";
    }
}

bool JWTUtil::addToBlacklist(const std::string& token) {
    try {
        std::string jti = getJTIFromToken(token);
        if (jti.empty()) {
            SYLAR_LOG_ERROR(g_logger) << "Cannot add token to blacklist: invalid JTI";
            return false;
        }
        
        time_t expire_time = getExpirationFromToken(token);
        if (expire_time == 0) {
            SYLAR_LOG_ERROR(g_logger) << "Cannot add token to blacklist: invalid expiration";
            return false;
        }
        
        // 添加到Redis黑名单
        bool redis_success = addToRedisBlacklist(jti, expire_time);
        
        // 添加到MySQL黑名单（用于持久化）
        int64_t user_id = getUserIdFromToken(token);
        bool mysql_success = addToMySQLBlacklist(jti, token, user_id, expire_time);
        
        if (redis_success || mysql_success) {
            SYLAR_LOG_DEBUG(g_logger) << "Added token to blacklist, JTI: " << jti;
            return true;
        }
        
        SYLAR_LOG_ERROR(g_logger) << "Failed to add token to both Redis and MySQL blacklist";
        return false;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to add token to blacklist: " << e.what();
        return false;
    }
}

bool JWTUtil::isInBlacklist(const std::string& token) {
    try {
        std::string jti = getJTIFromToken(token);
        if (jti.empty()) {
            return false;
        }
        
        // 先检查Redis（更快）
        if (isInRedisBlacklist(jti)) {
            return true;
        }
        
        // 如果Redis中没有，检查MySQL
        return isInMySQLBlacklist(jti);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to check token blacklist status: " << e.what();
        return false;
    }
}

void JWTUtil::cleanupExpiredBlacklist() {
    // 这里可以实现定期清理过期的黑名单记录
    // 实际实现需要根据具体的Redis和MySQL客户端来实现
    SYLAR_LOG_DEBUG(g_logger) << "Blacklist cleanup would be implemented here";
}

std::string JWTUtil::getCurrentSecret() {
    return current_secret_;
}

// 私有方法实现
std::string JWTUtil::getJWTSecret() {
    // 在实际应用中，应该从安全的配置源获取密钥
    // 这里使用随机生成的方式，生产环境应该使用固定的安全密钥
    
    const size_t secret_length = 64;
    unsigned char buffer[secret_length];
    
    if (RAND_bytes(buffer, secret_length) != 1) {
        // 如果OpenSSL RAND失败，使用随机设备作为后备
        std::random_device rd;
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < secret_length; ++i) {
            buffer[i] = static_cast<unsigned char>(dist(rd));
        }
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < secret_length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    
    return ss.str();
}

std::string JWTUtil::generateJTI() {
    // 生成UUID格式的JTI
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        int n = (i == 16) ? dis2(gen) : dis(gen);
        ss << n;
    }
    
    return ss.str();
}

bool JWTUtil::validateTokenStructure(const jwt::decoded_jwt& decoded_jwt) {
    try {
        // 检查必需的标准声明
        if (decoded_jwt.get_subject().empty()) {
            SYLAR_LOG_WARN(g_logger) << "Token missing subject claim";
            return false;
        }
        
        if (!decoded_jwt.has_expires_at()) {
            SYLAR_LOG_WARN(g_logger) << "Token missing expiration claim";
            return false;
        }
        
        if (decoded_jwt.get_id().empty()) {
            SYLAR_LOG_WARN(g_logger) << "Token missing JTI claim";
            return false;
        }
        
        // 检查自定义声明
        auto claims = decoded_jwt.get_payload_claims();
        if (claims.find("user_id") == claims.end()) {
            SYLAR_LOG_WARN(g_logger) << "Token missing user_id claim";
            return false;
        }
        
        if (claims.find("username") == claims.end()) {
            SYLAR_LOG_WARN(g_logger) << "Token missing username claim";
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Token structure validation failed: " << e.what();
        return false;
    }
}

std::optional<jwt::decoded_jwt> JWTUtil::safeDecode(const std::string& token) {
    try {
        return jwt::decode(token);
    } catch (const std::exception& e) {
        SYLAR_LOG_WARN(g_logger) << "Failed to decode JWT token: " << e.what();
        return std::nullopt;
    }
}

bool JWTUtil::addToRedisBlacklist(const std::string& jti, time_t expire_time) {
    try {
        // 伪代码：实际实现需要使用Redis客户端
        // redis_client->setex("jwt_blacklist:" + jti, expire_time - time(nullptr), "1");
        SYLAR_LOG_DEBUG(g_logger) << "Would add JTI " << jti << " to Redis blacklist with expiry " << expire_time;
        return true; // 模拟成功
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to add to Redis blacklist: " << e.what();
        return false;
    }
}

bool JWTUtil::isInRedisBlacklist(const std::string& jti) {
    try {
        // 伪代码：实际实现需要使用Redis客户端
        // return redis_client->exists("jwt_blacklist:" + jti);
        SYLAR_LOG_DEBUG(g_logger) << "Would check Redis blacklist for JTI: " << jti;
        return false; // 模拟不在黑名单中
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to check Redis blacklist: " << e.what();
        return false;
    }
}

bool JWTUtil::addToMySQLBlacklist(const std::string& jti, const std::string& token, 
                                int64_t user_id, time_t expire_time) {
    try {
        // 伪代码：实际实现需要使用数据库客户端
        // 执行INSERT语句将黑名单记录插入数据库
        SYLAR_LOG_DEBUG(g_logger) << "Would add JTI " << jti << " to MySQL blacklist for user " << user_id;
        return true; // 模拟成功
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to add to MySQL blacklist: " << e.what();
        return false;
    }
}

bool JWTUtil::isInMySQLBlacklist(const std::string& jti) {
    try {
        // 伪代码：实际实现需要使用数据库客户端
        // 执行SELECT查询检查JTI是否在黑名单中
        SYLAR_LOG_DEBUG(g_logger) << "Would check MySQL blacklist for JTI: " << jti;
        return false; // 模拟不在黑名单中
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to check MySQL blacklist: " << e.what();
        return false;
    }
}

} // namespace util
} // namespace blog_server