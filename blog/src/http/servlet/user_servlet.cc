#include "user_servlet.h"
#include "../../service/user_service.h"
#include "../../util/json_util.h"
#include "../../util/auth_util.h"
#include "../../model/response.h"

namespace blog_server {
namespace http {

// UserRegisterServlet 实现
UserRegisterServlet::UserRegisterServlet() : Servlet("UserRegisterServlet") {}

int32_t UserRegisterServlet::handle(sylar::http::HttpRequest::ptr request,
                                   sylar::http::HttpResponse::ptr response,
                                   sylar::http::HttpSession::ptr session) {
    // 解析请求体
    Json::Value json;
    if (!util::JsonUtil::parse(request->getBody(), json)) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "无效的JSON数据");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 获取请求参数
    std::string username = json["username"].asString();
    std::string password = json["password"].asString();
    std::string nickname = json["nickname"].asString();
    std::string email = json["email"].asString();
    
    // 参数验证
    if (username.empty() || password.empty()) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "用户名和密码不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    if (username.length() < 3 || username.length() > 20) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "用户名长度必须在3-20个字符之间");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    if (password.length() < 6) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "密码长度不能少于6个字符");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 调用服务层注册用户
    service::UserService userService;
    auto result = userService.registerUser(username, password, nickname, email);
    
    if (result.id == 0) {
        // 注册失败
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "用户名已存在或注册失败");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 注册成功，生成token
    std::string token = util::AuthUtil::generateToken(result);
    
    // 构建响应数据
    Json::Value data;
    data["token"] = token;
    data["user"] = result.toJson();
    
    model::ApiResponse api_response(200, "注册成功", data);
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

UserLoginServlet::UserLoginServlet() : Servlet("UserLoginServlet") {}

int32_t UserLoginServlet::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session) {
    Json::Value json;
    if (!util::JsonUtil::parse(request->getBody(), json)) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "无效的JSON数据");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    std::string username = json["username"].asString();
    std::string password = json["password"].asString();
    
    if (username.empty() || password.empty()) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "用户名和密码不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    service::UserService userService;
    auto user = userService.login(username, password);
    if (user.id == 0) {
        response->setStatus(sylar::http::HttpStatus::UNAUTHORIZED);
        model::ApiResponse api_response(401, "用户名或密码错误");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 生成token
    std::string token = util::AuthUtil::generateToken(user);
    
    Json::Value data;
    data["token"] = token;
    data["user"] = user.toJson();
    
    model::ApiResponse api_response(200, "登录成功", data);
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

UserInfoServlet::UserInfoServlet() : Servlet("UserInfoServlet") {}

int32_t UserInfoServlet::handle(sylar::http::HttpRequest::ptr request,
                               sylar::http::HttpResponse::ptr response,
                               sylar::http::HttpSession::ptr session) {
    // 从token获取用户信息
    model::User user = util::AuthUtil::getCurrentUser(request);
    if (user.id == 0) {
        response->setStatus(sylar::http::HttpStatus::UNAUTHORIZED);
        model::ApiResponse api_response(401, "请先登录");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 获取指定用户ID的信息（如果提供了user_id参数）
    std::string user_id_str = request->getParam("user_id");
    if (!user_id_str.empty()) {
        int64_t target_user_id = std::stol(user_id_str);
        service::UserService userService;
        auto target_user = userService.getUserById(target_user_id);
        if (target_user.id == 0) {
            response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
            model::ApiResponse api_response(404, "用户不存在");
            response->setBody(util::JsonUtil::toString(api_response.toJson()));
            response->setHeader("Content-Type", "application/json");
            return 0;
        }
        
        model::ApiResponse api_response(200, "success", target_user.toJson());
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 返回当前用户信息
    model::ApiResponse api_response(200, "success", user.toJson());
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

UserProfileServlet::UserProfileServlet() : Servlet("UserProfileServlet") {}

int32_t UserProfileServlet::handle(sylar::http::HttpRequest::ptr request,
                                  sylar::http::HttpResponse::ptr response,
                                  sylar::http::HttpSession::ptr session) {
    // 验证用户登录
    model::User current_user = util::AuthUtil::getCurrentUser(request);
    if (current_user.id == 0) {
        response->setStatus(sylar::http::HttpStatus::UNAUTHORIZED);
        model::ApiResponse api_response(401, "请先登录");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 解析请求体
    Json::Value json;
    if (!util::JsonUtil::parse(request->getBody(), json)) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "无效的JSON数据");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 更新用户信息
    model::User updated_user = current_user;
    if (json.isMember("nickname")) {
        updated_user.nickname = json["nickname"].asString();
    }
    if (json.isMember("email")) {
        updated_user.email = json["email"].asString();
    }
    if (json.isMember("avatar")) {
        updated_user.avatar = json["avatar"].asString();
    }
    if (json.isMember("bio")) {
        updated_user.bio = json["bio"].asString();
    }
    
    service::UserService userService;
    bool success = userService.updateUser(updated_user);
    if (!success) {
        response->setStatus(sylar::http::HttpStatus::INTERNAL_SERVER_ERROR);
        model::ApiResponse api_response(500, "更新失败");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    model::ApiResponse api_response(200, "资料更新成功", updated_user.toJson());
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

} // namespace http
} // namespace blog_server