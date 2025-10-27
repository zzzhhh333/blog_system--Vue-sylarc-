// sylar/db/mysql.cpp
#include "mysql.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/env.h"
#include <yaml-cpp/yaml.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool MySQLConf::loadFromYaml(const YAML::Node& node) {
    try {
        host = node["host"].as<std::string>();
        port = node["port"].as<uint32_t>();
        user = node["user"].as<std::string>();
        password = node["password"].as<std::string>();
        database = node["database"].as<std::string>();
        
        if(node["charset"]) {
            charset = node["charset"].as<std::string>();
        }
        if(node["timeout"]) {
            timeout = node["timeout"].as<uint32_t>();
        }
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load MySQL config error: " << e.what();
        return false;
    }
}

MySQLPool::MySQLPool(const MySQLConf& conf, uint32_t max_conn)
    : m_conf(conf), m_maxConn(max_conn) {
    
    // 预先创建连接
    for(uint32_t i = 0; i < max_conn / 2; ++i) {
        auto conn = std::make_shared<MySQLWrapper>();
        if(conn->connect(conf.host, conf.user, conf.password, 
                        conf.database, conf.port, conf.charset)) {
            m_conns.push_back(conn);
        } else {
            SYLAR_LOG_ERROR(g_logger) << "Connect to MySQL failed: " << conn->getError();
        }
    }
}

MySQLPool::~MySQLPool() {
    // 连接会在MySQLWrapper析构时自动关闭
}

std::shared_ptr<MySQLWrapper> MySQLPool::getConnection() {
    RWMutexType::WriteLock lock(m_mutex);
    
    // 如果有可用连接，直接返回
    if(!m_conns.empty()) {
        auto conn = m_conns.back();
        m_conns.pop_back();
        return conn;
    }
    
    // 没有可用连接，创建新连接
    auto conn = std::make_shared<MySQLWrapper>();
    if(conn->connect(m_conf.host, m_conf.user, m_conf.password,
                    m_conf.database, m_conf.port, m_conf.charset)) {
        return conn;
    }
    
    SYLAR_LOG_ERROR(g_logger) << "Create MySQL connection failed: " << conn->getError();
    return nullptr;
}

void MySQLPool::returnConnection(std::shared_ptr<MySQLWrapper> conn) {
    if(!conn || !conn->isConnected()) {
        return;
    }
    
    RWMutexType::WriteLock lock(m_mutex);
    
    // 如果连接池未满，归还连接；否则直接销毁
    if(m_conns.size() < m_maxConn) {
        m_conns.push_back(conn);
    }
}

void MySQLPool::checkConnections() {
    RWMutexType::WriteLock lock(m_mutex);
    
    // 检查连接是否有效
    for(auto it = m_conns.begin(); it != m_conns.end();) {
        if(!(*it)->isConnected()) {
            it = m_conns.erase(it);
        } else {
            ++it;
        }
    }
}

bool MySQLManager::Init(const std::string& conf_path) {
    std::string path = conf_path;
    if(path.empty()) {
        path = sylar::EnvMgr::GetInstance()->getConfigPath();
    }
    
    try {
        YAML::Node root = YAML::LoadFile(path + "/mysql.yml");
        for(auto it = root.begin(); it != root.end(); ++it) {
            std::string name = it->first.as<std::string>();
            YAML::Node node = it->second;
            
            MySQLConf conf;
            if(conf.loadFromYaml(node)) {
                uint32_t max_conn = node["max_conn"] ? node["max_conn"].as<uint32_t>() : 10;
                auto pool = std::make_shared<MySQLPool>(conf, max_conn);
                GetInstance()->addPool(name, pool);
                
                SYLAR_LOG_INFO(g_logger) << "Init MySQL pool: " << name 
                                       << " " << conf.host << ":" << conf.port 
                                       << "/" << conf.database;
            }
        }
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load MySQL config error: " << e.what();
        return false;
    }
}

MySQLManager* MySQLManager::GetInstance() {
    static MySQLManager instance;
    return &instance;
}

MySQLPool::ptr MySQLManager::getPool(const std::string& name) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_pools.find(name);
    return it != m_pools.end() ? it->second : nullptr;
}

std::shared_ptr<MySQLWrapper> MySQLManager::getConnection(const std::string& name) {
    auto pool = getPool(name);
    if(pool) {
        return pool->getConnection();
    }
    return nullptr;
}

void MySQLManager::addPool(const std::string& name, MySQLPool::ptr pool) {
    RWMutexType::WriteLock lock(m_mutex);
    m_pools[name] = pool;
}

} // namespace sylar