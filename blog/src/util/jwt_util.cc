// blog_server/util/jwt_util.cpp
#include "jwt_util.h"
#include "sylar/config.h"
#include "sylar/env.h"
#include "sylar/log.h"
#include "sylar/db/redis.h"
#include "sylar/db/mysql.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace blog_server {
namespace util {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 静态成员变量定义
std::string JWTUtil::current_secret_;
std::string JWTUtil::previous_secret_;
time_t JWTUtil::secret_rotation_time_ = 0;

// 生成随机字符串
std::string JWTUtil::generateRandomString(size_t length) {
    const std::string chars = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string random_string;
    random_string.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        random_string += chars[distribution(generator)];
    }
    
    return random_string;
}

// Base64 URL 编码字符表
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string JWTUtil::base64UrlEncode(const std::string& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = data.size();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(data.c_str());

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    // 转换为 URL安全的 Base64
    size_t pos;
    while ((pos = ret.find('+')) != std::string::npos) {
        ret.replace(pos, 1, "-");
    }
    while ((pos = ret.find('/')) != std::string::npos) {
        ret.replace(pos, 1, "_");
    }
    while ((pos = ret.find('=')) != std::string::npos) {
        ret.erase(pos, 1);
    }

    return ret;
}

std::string JWTUtil::base64UrlDecode(const std::string& encoded_string) {
    std::string input = encoded_string;
    
    // 转换回标准Base64
    size_t pos;
    while ((pos = input.find('-')) != std::string::npos) {
        input.replace(pos, 1, "+");
    }
    while ((pos = input.find('_')) != std::string::npos) {
        input.replace(pos, 1, "/");
    }
    
    // 添加padding
    int padding = input.length() % 4;
    if (padding > 0) {
        input.append(4 - padding, '=');
    }

    size_t in_len = input.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( input[in_] != '=') && is_base64(input[in_])) {
        char_array_4[i++] = input[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}

std::string JWTUtil::hmacSha256(const std::string& key, const std::string& data) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), key.c_str(), key.length(), 
                  reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), NULL, NULL);
    
    std::string result(reinterpret_cast<char*>(digest), 32);
    return base64UrlEncode(result);
}

std::string JWTUtil::serializeClaims(const JWTClaims& claims) {
    std::stringstream ss;
    ss << "{";
    ss << "\"sub\":\"" << claims.user_id << "\",";
    ss << "\"username\":\"" << claims.username << "\",";
    ss << "\"jti\":\"" << claims.jti << "\",";
    ss << "\"iat\":" << claims.iat << ",";
    ss << "\"exp\":" << claims.exp << ",";
    ss << "\"iss\":\"" << claims.issuer << "\"";
    
    if (!claims.roles.empty()) {
        ss << ",\"roles\":[";
        for (size_t i = 0; i < claims.roles.size(); ++i) {
            ss << "\"" << claims.roles[i] << "\"";
            if (i < claims.roles.size() - 1) {
                ss << ",";
            }
        }
        ss << "]";
    }
    ss << "}";
    return ss.str();
}

bool JWTUtil::parseClaims(const std::string& json, JWTClaims& claims) {
    try {
        // 简单的JSON解析，实际项目中可以使用sylar的JSON库或第三方JSON库
        // 这里简化处理，只提取关键字段
        
        // 提取user_id (sub)
        size_t pos = json.find("\"sub\":\"");
        if (pos == std::string::npos) return false;
        pos += 7;
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return false;
        claims.user_id = std::stoll(json.substr(pos, end - pos));
        
        // 提取username
        pos = json.find("\"username\":\"", end);
        if (pos == std::string::npos) return false;
        pos += 12;
        end = json.find("\"", pos);
        if (end == std::string::npos) return false;
        claims.username = json.substr(pos, end - pos);
        
        // 提取jti
        pos = json.find("\"jti\":\"", end);
        if (pos == std::string::npos) return false;
        pos += 7;
        end = json.find("\"", pos);
        if (end == std::string::npos) return false;
        claims.jti = json.substr(pos, end - pos);
        
        // 提取iat
        pos = json.find("\"iat\":", end);
        if (pos == std::string::npos) return false;
        pos += 6;
        end = json.find(",", pos);
        if (end == std::string::npos) return false;
        claims.iat = std::stoll(json.substr(pos, end - pos));
        
        // 提取exp
        pos = json.find("\"exp\":", end);
        if (pos == std::string::npos) return false;
        pos += 6;
        end = json.find(",", pos);
        if (end == std::string::npos) {
            end = json.find("}", pos);
            if (end == std::string::npos) return false;
        }
        claims.exp = std::stoll(json.substr(pos, end - pos));
        
        // 提取issuer
        pos = json.find("\"iss\":\"", end);
        if (pos != std::string::npos) {
            pos += 7;
            end = json.find("\"", pos);
            if (end != std::string::npos) {
                claims.issuer = json.substr(pos, end - pos);
            }
        }
        
        // 提取roles
        pos = json.find("\"roles\":[", end);
        if (pos != std::string::npos) {
            pos += 9;
            end = json.find("]", pos);
            if (end != std::string::npos) {
                std::string roles_str = json.substr(pos, end - pos);
                size_t start = 0;
                while (start < roles_str.length()) {
                    size_t quote_start = roles_str.find("\"", start);
                    if (quote_start == std::string::npos) break;
                    size_t quote_end = roles_str.find("\"", quote_start + 1);
                    if (quote_end == std::string::npos) break;
                    
                    std::string role = roles_str.substr(quote_start + 1, quote_end - quote_start - 1);
                    claims.roles.push_back(role);
                    start = quote_end + 2; // 跳过逗号和空格
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Parse claims failed: " << e.what();
        return false;
    }
}

bool JWTUtil::init() {
    // 从配置中获取JWT密钥
    auto secret = sylar::EnvMgr::GetInstance()->getEnv("JWT_SECRET");
    if (secret.empty()) {
        auto jwt_secret = sylar::Config::Lookup("jwt.secret", 
            std::string("blog_server_jwt_secret_key_2024"), "JWT secret key");
        if (jwt_secret) {
            secret = jwt_secret->toString();
        } else {
            secret = "blog_server_jwt_secret_key_2024";
        }
    }
    
    current_secret_ = secret;
    previous_secret_ = secret;
    secret_rotation_time_ = time(nullptr);
    
    SYLAR_LOG_INFO(g_logger) << "JWTUtil initialized successfully";
    return true;
}

std::string JWTUtil::generateToken(int64_t user_id, 
                                 const std::string& username, 
                                 const std::vector<std::string>& roles,
                                 int64_t expires_in) {
    try {
        JWTClaims claims;
        claims.user_id = user_id;
        claims.username = username;
        claims.roles = roles;
        claims.jti = generateJTI();
        claims.iat = time(nullptr);
        claims.exp = claims.iat + expires_in;
        claims.issuer = "blog_server";
        
        // 构建Header
        std::string header = R"({"alg":"HS256","typ":"JWT"})";
        std::string header_encoded = base64UrlEncode(header);
        
        // 构建Payload
        std::string payload = serializeClaims(claims);
        std::string payload_encoded = base64UrlEncode(payload);
        
        // 计算签名
        std::string data = header_encoded + "." + payload_encoded;
        std::string signature = hmacSha256(current_secret_, data);
        
        // 组合JWT令牌
        return data + "." + signature;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Generate token failed: " << e.what();
        return "";
    }
}

bool JWTUtil::parseToken(const std::string& token, JWTClaims& claims, std::string& signature) {
    try {
        // 分割令牌
        size_t dot1 = token.find('.');
        size_t dot2 = token.find('.', dot1 + 1);
        
        if (dot1 == std::string::npos || dot2 == std::string::npos) {
            return false;
        }
        
        std::string header_encoded = token.substr(0, dot1);
        std::string payload_encoded = token.substr(dot1 + 1, dot2 - dot1 - 1);
        signature = token.substr(dot2 + 1);
        
        // 解码Header
        std::string header = base64UrlDecode(header_encoded);
        if (header.find("HS256") == std::string::npos) {
            return false; // 不支持的算法
        }
        
        // 解码Payload
        std::string payload = base64UrlDecode(payload_encoded);
        if (!parseClaims(payload, claims)) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Parse token failed: " << e.what();
        return false;
    }
}

bool JWTUtil::verifyToken(const std::string& token, JWTClaims& claims) {
    try {
        // 先检查黑名单
        if (isInBlacklist(token)) {
            SYLAR_LOG_WARN(g_logger) << "Token is in blacklist";
            return false;
        }
        
        std::string signature;
        if (!parseToken(token, claims, signature)) {
            return false;
        }
        
        // 验证令牌结构
        if (!validateTokenStructure(claims)) {
            return false;
        }
        
        // 验证过期时间
        if (claims.isExpired()) {
            SYLAR_LOG_WARN(g_logger) << "Token expired";
            return false;
        }
        
        // 重新计算签名并验证
        size_t dot2 = token.find_last_of('.');
        std::string data = token.substr(0, dot2);
        
        // 用当前密钥验证
        std::string expected_signature = hmacSha256(current_secret_, data);
        if (expected_signature == signature) {
            return true;
        }
        
        // 如果当前密钥验证失败，尝试用上一个密钥验证
        if (!previous_secret_.empty() && previous_secret_ != current_secret_) {
            expected_signature = hmacSha256(previous_secret_, data);
            if (expected_signature == signature) {
                return true;
            }
        }
        
        SYLAR_LOG_WARN(g_logger) << "Token signature verification failed";
        return false;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Token verification failed: " << e.what();
        return false;
    }
}

bool JWTUtil::verifyToken(const std::string& token) {
    JWTClaims claims;
    return verifyToken(token, claims);
}

int64_t JWTUtil::getUserIdFromToken(const std::string& token) {
    auto claims = safeDecode(token);
    return claims ? claims->user_id : 0;
}

std::string JWTUtil::getUsernameFromToken(const std::string& token) {
    auto claims = safeDecode(token);
    return claims ? claims->username : "";
}

std::vector<std::string> JWTUtil::getRolesFromToken(const std::string& token) {
    auto claims = safeDecode(token);
    return claims ? claims->roles : std::vector<std::string>{};
}

std::string JWTUtil::getJTIFromToken(const std::string& token) {
    auto claims = safeDecode(token);
    return claims ? claims->jti : "";
}

time_t JWTUtil::getExpirationFromToken(const std::string& token) {
    auto claims = safeDecode(token);
    return claims ? claims->exp : 0;
}

std::string JWTUtil::refreshToken(const std::string& token, int64_t expires_in) {
    JWTClaims claims;
    if (!verifyToken(token, claims)) {
        return "";
    }
    
    // 生成新令牌
    auto new_token = generateToken(claims.user_id, claims.username, claims.roles, expires_in);
    
    // 将原令牌加入黑名单
    addToBlacklist(token);
    
    return new_token;
}

bool JWTUtil::validateTokenStructure(const JWTClaims& claims) {
    return claims.user_id > 0 && 
           !claims.username.empty() && 
           !claims.jti.empty() && 
           claims.iat > 0 && 
           claims.exp > claims.iat;
}

std::string JWTUtil::generateJTI() {
    // 生成随机的JTI
    unsigned char buffer[16];
    if (RAND_bytes(buffer, sizeof(buffer)) == 1) {
        std::stringstream ss;
        for (int i = 0; i < 16; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
        }
        return ss.str();
    } else {
        // 如果RAND_bytes失败，使用我们自己的随机字符串生成器
        SYLAR_LOG_WARN(g_logger) << "RAND_bytes failed, using fallback random string generator";
        return generateRandomString(32);
    }
}

std::optional<JWTClaims> JWTUtil::safeDecode(const std::string& token) {
    try {
        JWTClaims claims;
        std::string signature;
        if (parseToken(token, claims, signature)) {
            return claims;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Safe decode token failed: " << e.what();
        return std::nullopt;
    }
}

// 黑名单管理方法
bool JWTUtil::addToBlacklist(const std::string& token) {
    try {
        auto claims = safeDecode(token);
        if (!claims) {
            return false;
        }
        
        auto jti = claims->jti;
        auto expire_time = claims->exp;
        
        // 添加到Redis黑名单
        bool redis_success = addToRedisBlacklist(jti, expire_time);
        
        // 添加到MySQL黑名单（持久化）
        bool mysql_success = addToMySQLBlacklist(jti, token, claims->user_id, expire_time);
        
        SYLAR_LOG_INFO(g_logger) << "Token added to blacklist, jti: " << jti 
                                << ", redis: " << redis_success 
                                << ", mysql: " << mysql_success;
        
        return redis_success || mysql_success;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Add token to blacklist failed: " << e.what();
        return false;
    }
}

bool JWTUtil::isInBlacklist(const std::string& token) {
    try {
        auto jti = getJTIFromToken(token);
        if (jti.empty()) {
            return false;
        }
        
        // 先检查Redis
        if (isInRedisBlacklist(jti)) {
            return true;
        }
        
        // 再检查MySQL
        return isInMySQLBlacklist(jti);
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Check token in blacklist failed: " << e.what();
        return false;
    }
}

void JWTUtil::cleanupExpiredBlacklist() {
    SYLAR_LOG_INFO(g_logger) << "Cleanup expired blacklist tokens";
}

std::string JWTUtil::getCurrentSecret() {
    return current_secret_;
}

bool JWTUtil::addToRedisBlacklist(const std::string& jti, time_t expire_time) {
    try {
        auto redis = sylar::RedisMgr::GetInstance()->getConnection();
        if (!redis) {
            SYLAR_LOG_WARN(g_logger) << "Redis connection not available";
            return false;
        }
        
        auto key = "jwt_blacklist:" + jti;
        auto ttl = expire_time - time(nullptr);
        if (ttl <= 0) {
            return true;
        }
        
        auto result = redis->setex(key, ttl, "1");
        return result;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Add to redis blacklist failed: " << e.what();
        return false;
    }
}

bool JWTUtil::isInRedisBlacklist(const std::string& jti) {
    try {
        auto redis = sylar::RedisMgr::GetInstance()->getConnection();
        if (!redis) {
            return false;
        }
        
        auto key = "jwt_blacklist:" + jti;
        auto result = redis->exists(key);
        return result;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Check redis blacklist failed: " << e.what();
        return false;
    }
}

bool JWTUtil::addToMySQLBlacklist(const std::string& jti, const std::string& token, 
                                 int64_t user_id, time_t expire_time) {
    try {
        auto db = sylar::MySQLMgr::GetInstance()->getConnection();
        if (!db) {
            SYLAR_LOG_WARN(g_logger) << "MySQL connection not available";
            return false;
        }
        
        auto now = time(nullptr);
        auto stmt = db->prepare(
            "INSERT INTO jwt_blacklist (jti, token, user_id, create_time, expire_time) "
            "VALUES (?, ?, ?, ?, ?)"
        );
        
        if (!stmt) {
            return false;
        }
        
        stmt->setString(1, jti);
        stmt->setString(2, token);
        stmt->setInt(3, user_id);
        stmt->setInt(4, now);
        stmt->setInt(5, expire_time);
        
        auto result = stmt->execute();
        return result;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Add to MySQL blacklist failed: " << e.what();
        return false;
    }
}

bool JWTUtil::isInMySQLBlacklist(const std::string& jti) {
    try {
        auto db = sylar::MySQLMgr::GetInstance()->getConnection();
        if (!db) {
            return false;
        }
        
        auto stmt = db->prepare(
            "SELECT COUNT(*) FROM jwt_blacklist WHERE jti = ? AND expire_time > ?"
        );
        
        if (!stmt) {
            return false;
        }
        
        auto now = time(nullptr);
        stmt->setString(1, jti);
        stmt->setInt(2, now);
        
        auto result = stmt->query();
        return result?true : false;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Check MySQL blacklist failed: " << e.what();
        return false;
    }
}

} // namespace util
} // namespace blog_server