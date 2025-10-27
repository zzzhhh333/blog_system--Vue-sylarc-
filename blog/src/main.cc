#include "sylar/sylar.h"
#include "core/server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run()
{
    // 加载配置文件
    sylar::Config::LoadFromConfDir("config/blog_server.json");
    
    // 设置日志级别
    g_logger->setLevel(sylar::LogLevel::INFO);
    
    // 创建服务器实例
    blog_server::BlogServer::ptr server(new blog_server::BlogServer);
    
    // 初始化服务器
    if (!server->init()) {
        SYLAR_LOG_ERROR(g_logger) << "Server init failed";
        return;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Blog server starting...";
    
    // 启动服务器
    server->start();
    
}

int main(int argc, char** argv) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());
    
    sylar::IOManager iom(1, false, "blog_server");
    iom.schedule(run);
    return 0;
}