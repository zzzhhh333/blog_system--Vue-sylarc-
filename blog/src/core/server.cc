#include "server.h"
#include "sylar/env.h"
#include "../http/servlet/blog_servlet.h"
#include "../http/servlet/user_servlet.h"
#include "../util/json_util.h"

namespace blog_server {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("blog");

BlogServer::BlogServer() {
    m_httpServer = std::make_shared<sylar::http::HttpServer>();
}

bool BlogServer::init() {
    // 获取配置的端口
    auto port = sylar::Config::Lookup("server.port", std::string("8081"))->getValue();
    auto addr = sylar::Address::LookupAnyIPAddress("localhost:" + port);
    
    if (!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address fail";
        return false;
    }
    
    // 绑定地址
    if (!m_httpServer->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "bind http server fail";
        return false;
    }
    
    // 注册Servlet
    registerServlets();
    
    // 初始化数据库连接
    initDatabase();
    
    SYLAR_LOG_INFO(g_logger) << "BlogServer init success, listening on " << addr->toString();
    return true;
}

void BlogServer::registerServlets() {
    auto dispatch = m_httpServer->getServletDispatch();
    
    // 博客相关接口
    dispatch->addServlet("/api/blogs", std::make_shared<blog_server::http::BlogListServlet>());
    dispatch->addServlet("/api/blogs/user", std::make_shared<blog_server::http::BlogListWithUserIdServlet>());
    dispatch->addServlet("/api/blogs/create", std::make_shared<blog_server::http::BlogCreateServlet>());
    dispatch->addServlet("/api/blogs/update", std::make_shared<blog_server::http::BlogUpdateServlet>());
    dispatch->addServlet("/api/blogs/delete", std::make_shared<blog_server::http::BlogDeleteServlet>());
    dispatch->addServlet("/api/blogs/detail", std::make_shared<blog_server::http::BlogDetailServlet>());
    
    // 用户相关接口
    dispatch->addServlet("/api/users/register", std::make_shared<blog_server::http::UserRegisterServlet>());
    dispatch->addServlet("/api/users/login", std::make_shared<blog_server::http::UserLoginServlet>());
    dispatch->addServlet("/api/users/info", std::make_shared<blog_server::http::UserInfoServlet>());
    dispatch->addServlet("/api/users/profile", std::make_shared<blog_server::http::UserProfileServlet>());
    
    // 设置默认的404处理器
    auto notFoundServlet = std::make_shared<sylar::http::NotFoundServlet>("blog_server");
    dispatch->setDefault(notFoundServlet);
}

void BlogServer::initDatabase() {
    // 这里可以初始化数据库连接
    // 实际项目中可以使用MySQL、SQLite等

    SYLAR_LOG_INFO(g_logger) << "Database initialized connected...";

    if(!sylar::db::MySQLManagerSingleton::GetInstance()->Init()) 
    {
        SYLAR_LOG_ERROR(g_logger) << "Failed to initialize MySQLManager";
        return;
    }

    if(!sylar::db::RedisManagerSingleton::GetInstance()->Init()) 
    {
        SYLAR_LOG_ERROR(g_logger) << "Failed to initialize RedisManager";
        return;
    }
    

    SYLAR_LOG_INFO(g_logger) << "MySQL and Redis Managers initialized successfully";
}

void BlogServer::start() {
    m_httpServer->start();
}

void BlogServer::stop() {
    m_httpServer->stop();
}

} // namespace blog_server