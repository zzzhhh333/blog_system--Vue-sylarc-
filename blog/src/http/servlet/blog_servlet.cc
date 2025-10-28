#include "blog_servlet.h"
#include "../../service/blog_service.h"
#include "../../service/user_service.h"
#include "../../util/json_util.h"
#include "../../util/auth_util.h"
#include "../../model/response.h"
#include <sstream>

namespace blog_server {
namespace http {

BlogListServlet::BlogListServlet() : Servlet("BlogListServlet") {}

int32_t BlogListServlet::handle(sylar::http::HttpRequest::ptr request,
                               sylar::http::HttpResponse::ptr response,
                               sylar::http::HttpSession::ptr session) {
    // 获取查询参数
    int page = std::stoi(request->getParam("page", "1"));
    int page_size = std::stoi(request->getParam("page_size", "20"));
    int status = std::stoi(request->getParam("status", "1")); // 默认只查已发布的
    
    // 调用服务层
    service::BlogService blog_service;
    auto blogs = blog_service.getBlogList(page, page_size, status);
    
    // 构建响应
    Json::Value data(Json::arrayValue);
    for (const auto& blog : blogs) {
        data.append(blog.toJson());
    }
    
    model::ApiResponse api_response(200, "success", data);
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

BlogListWithUserIdServlet::BlogListWithUserIdServlet() : Servlet("BlogListWithUserNameServlet") {}

int32_t BlogListWithUserIdServlet::handle(sylar::http::HttpRequest::ptr request,
                               sylar::http::HttpResponse::ptr response,
                               sylar::http::HttpSession::ptr session) {
    // 获取查询参数
    int page = std::stoi(request->getParam("page", "1"));
    int page_size = std::stoi(request->getParam("page_size", "20"));
    int user_id = std::stoi(request->getParam("user_id","0"));
    
    if (user_id == 0) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "用户id不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 调用服务层
    service::BlogService blog_service;
    auto blogs = blog_service.getUserBlogs(user_id, page, page_size);
    
    // 构建响应
    Json::Value data(Json::arrayValue);
    for (const auto& blog : blogs) {
        data.append(blog.toJson());
    }
    
    model::ApiResponse api_response(200, "success", data);
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

BlogCreateServlet::BlogCreateServlet() : Servlet("BlogCreateServlet") {}

int32_t BlogCreateServlet::handle(sylar::http::HttpRequest::ptr request,
                                 sylar::http::HttpResponse::ptr response,
                                 sylar::http::HttpSession::ptr session) {
    // 验证用户登录
    auto user = util::AuthUtil::getCurrentUser(request);
    if (user.id == 0) {
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
    
    // 验证必要字段
    if (!json.isMember("title") || !json.isMember("content")) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "标题和内容不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 创建博客
    model::Blog blog = model::Blog::fromJson(json);
    blog.author_id = user.id;
    blog.author_name = user.nickname;
    
    // 生成摘要（取前100个字符）
    if (blog.summary.empty() && blog.content.length() > 100) {
        blog.summary = blog.content.substr(0, 100) + "...";
    }
    
    service::BlogService blog_service;
    auto new_blog = blog_service.createBlog(blog);
    
    // 返回创建结果
    model::ApiResponse api_response(200, "博客创建成功", new_blog.toJson());
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

BlogUpdateServlet::BlogUpdateServlet() : Servlet("BlogUpdateServlet") {}

int32_t BlogUpdateServlet::handle(sylar::http::HttpRequest::ptr request,
                                 sylar::http::HttpResponse::ptr response,
                                 sylar::http::HttpSession::ptr session) {
    // 验证用户登录
    auto user = util::AuthUtil::getCurrentUser(request);
    if (user.id == 0) {
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
    
    if (!json.isMember("id")) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "博客ID不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 检查博客是否存在且属于当前用户
    service::BlogService blog_service;
    auto existing_blog = blog_service.getBlog(json["id"].asInt64());
    if (existing_blog.id == 0) {
        response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
        model::ApiResponse api_response(404, "博客不存在");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    if (existing_blog.author_id != user.id) {
        response->setStatus(sylar::http::HttpStatus::FORBIDDEN);
        model::ApiResponse api_response(403, "无权修改此博客");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 更新博客
    model::Blog blog = model::Blog::fromJson(json);
    blog.id = existing_blog.id;
    blog.author_id = existing_blog.author_id;
    blog.author_name = existing_blog.author_name;
    
    bool success = blog_service.updateBlog(blog);
    if (!success) {
        response->setStatus(sylar::http::HttpStatus::INTERNAL_SERVER_ERROR);
        model::ApiResponse api_response(500, "更新失败");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    model::ApiResponse api_response(200, "博客更新成功");
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

BlogDeleteServlet::BlogDeleteServlet() : Servlet("BlogDeleteServlet") {}

int32_t BlogDeleteServlet::handle(sylar::http::HttpRequest::ptr request,
                                 sylar::http::HttpResponse::ptr response,
                                 sylar::http::HttpSession::ptr session) {
    // 验证用户登录
    auto user = util::AuthUtil::getCurrentUser(request);
    if (user.id == 0) {
        response->setStatus(sylar::http::HttpStatus::UNAUTHORIZED);
        model::ApiResponse api_response(401, "请先登录");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 获取博客ID
    int64_t blog_id = std::stol(request->getParam("id", "0"));
    if (blog_id == 0) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "博客ID不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 删除博客
    service::BlogService blog_service;
    bool success = blog_service.deleteBlog(blog_id, user.id);
    if (!success) {
        response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
        model::ApiResponse api_response(404, "博客不存在或无权限删除");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    model::ApiResponse api_response(200, "博客删除成功");
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

BlogDetailServlet::BlogDetailServlet() : Servlet("BlogDetailServlet") {}

int32_t BlogDetailServlet::handle(sylar::http::HttpRequest::ptr request,
                                 sylar::http::HttpResponse::ptr response,
                                 sylar::http::HttpSession::ptr session) {
    // 获取博客ID
    int64_t blog_id = std::stol(request->getParam("id", "0"));
    if (blog_id == 0) {
        response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
        model::ApiResponse api_response(400, "博客ID不能为空");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 获取博客详情
    service::BlogService blog_service;
    auto blog = blog_service.getBlog(blog_id);
    if (blog.id == 0) {
        response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
        model::ApiResponse api_response(404, "博客不存在");
        response->setBody(util::JsonUtil::toString(api_response.toJson()));
        response->setHeader("Content-Type", "application/json");
        return 0;
    }
    
    // 增加阅读量
    blog_service.increaseViewCount(blog_id);
    
    model::ApiResponse api_response(200, "success", blog.toJson());
    response->setBody(util::JsonUtil::toString(api_response.toJson()));
    response->setHeader("Content-Type", "application/json");
    
    return 0;
}

} // namespace http
} // namespace blog_server