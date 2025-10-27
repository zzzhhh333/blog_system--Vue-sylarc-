#ifndef BLOG_SERVER_CORE_SERVER_H
#define BLOG_SERVER_CORE_SERVER_H

#include "sylar/http/http_server.h"
#include "sylar/db/mysql.h"
#include "sylar/db/redis.h"
#include <memory>

namespace blog_server {

class BlogServer {
public:
    typedef std::shared_ptr<BlogServer> ptr;
    
    BlogServer();
    ~BlogServer() = default;
    
    bool init();
    void start();
    void stop();

private:
    void registerServlets();
    void initDatabase();
    
private:
    sylar::http::HttpServer::ptr m_httpServer;
    sylar::MySQLPool::ptr m_mysqlPool;
    sylar::RedisPool::ptr m_redisPool;
};

} // namespace blog_server

#endif