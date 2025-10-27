// sylar/db/redis.cpp
#include "redis.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/env.h"
#include <yaml-cpp/yaml.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool RedisConf::loadFromYaml(const YAML::Node& node) {
    try {
        host = node["host"].as<std::string>();
        port = node["port"].as<uint32_t>();
        
        if(node["password"]) {
            password = node["password"].as<std::string>();
        }
        if(node["timeout"]) {
            timeout = node["timeout"].as<uint32_t>();
        }
        if(node["pool_size"]) {
            pool_size = node["pool_size"].as<uint32_t>();
        }
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load Redis config error: " << e.what();
        return false;
    }
}

RedisPool::RedisPool(const RedisConf& conf)
    : m_conf(conf) {
    
    SYLAR_LOG_INFO(g_logger) << "Creating Redis pool for " << conf.host << ":" << conf.port 
                            << " with size: " << conf.pool_size;
    
    // 预先创建连接
    for(uint32_t i = 0; i < conf.pool_size; ++i) {
        auto conn = createNewConnection();
        if(conn) {
            m_conns.push_back(conn);
            SYLAR_LOG_DEBUG(g_logger) << "Redis connection " << (i + 1) << "/" << conf.pool_size << " created";
        } else {
            SYLAR_LOG_ERROR(g_logger) << "Failed to create Redis connection " << (i + 1) << "/" << conf.pool_size;
        }
    }
    
    SYLAR_LOG_INFO(g_logger) << "Redis pool created with " << m_conns.size() << "/" << conf.pool_size << " connections";
}

RedisPool::~RedisPool() {
    SYLAR_LOG_DEBUG(g_logger) << "Destroying Redis pool for " << m_conf.host << ":" << m_conf.port;
    // 连接会在RedisWrapper析构时自动关闭
}

std::shared_ptr<RedisWrapper> RedisPool::getConnection() {
    RWMutexType::WriteLock lock(m_mutex);
    
    SYLAR_LOG_DEBUG(g_logger) << "Getting Redis connection from pool, current size: " << m_conns.size();
    
    // 如果有可用连接，直接返回
    if(!m_conns.empty()) {
        auto conn = m_conns.back();
        m_conns.pop_back();
        
        SYLAR_LOG_DEBUG(g_logger) << "Got connection from pool, checking health...";
        
        // 检查连接是否有效
        if(conn->isConnected() && conn->ping()) {
            SYLAR_LOG_DEBUG(g_logger) << "Connection is healthy, returning";
            return conn;
        } else {
            SYLAR_LOG_WARN(g_logger) << "Connection is unhealthy, creating new one";
            // 连接无效，创建新连接
            return createNewConnection();
        }
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "No available connections in pool, creating new one";
    // 没有可用连接，创建新连接
    return createNewConnection();
}

std::shared_ptr<RedisWrapper> RedisPool::createNewConnection() {
    SYLAR_LOG_DEBUG(g_logger) << "Creating new Redis connection to " << m_conf.host << ":" << m_conf.port;
    
    auto conn = std::make_shared<RedisWrapper>();
    bool success = false;
    
    if(m_conf.password.empty()) {
        success = conn->connect(m_conf.host, m_conf.port, m_conf.timeout);
    } else {
        success = conn->connectWithAuth(m_conf.host, m_conf.port, m_conf.password, m_conf.timeout);
    }
    
    if(success) {
        SYLAR_LOG_INFO(g_logger) << "New Redis connection created successfully to " 
                               << m_conf.host << ":" << m_conf.port;
        return conn;
    }
    
    SYLAR_LOG_ERROR(g_logger) << "Create Redis connection failed: " << conn->getError()
                             << " for " << m_conf.host << ":" << m_conf.port;
    return nullptr;
}

void RedisPool::returnConnection(std::shared_ptr<RedisWrapper> conn) {
    if(!conn) {
        SYLAR_LOG_WARN(g_logger) << "Attempt to return null Redis connection";
        return;
    }
    
    if(!conn->isConnected()) {
        SYLAR_LOG_WARN(g_logger) << "Attempt to return disconnected Redis connection";
        return;
    }
    
    RWMutexType::WriteLock lock(m_mutex);
    
    // 如果连接池未满，归还连接；否则直接销毁
    if(m_conns.size() < m_conf.pool_size) {
        m_conns.push_back(conn);
        SYLAR_LOG_DEBUG(g_logger) << "Redis connection returned to pool, current size: " << m_conns.size();
    } else {
        SYLAR_LOG_DEBUG(g_logger) << "Redis pool is full, connection destroyed";
        // 连接会在conn离开作用域时自动销毁
    }
}

void RedisPool::checkConnections() {
    RWMutexType::WriteLock lock(m_mutex);
    
    SYLAR_LOG_DEBUG(g_logger) << "Checking Redis connections health, current count: " << m_conns.size();
    
    size_t initial_size = m_conns.size();
    size_t removed_count = 0;
    
    // 检查连接是否有效
    for(auto it = m_conns.begin(); it != m_conns.end();) {
        if(!(*it)->isConnected() || !(*it)->ping()) {
            SYLAR_LOG_WARN(g_logger) << "Removing unhealthy Redis connection";
            it = m_conns.erase(it);
            removed_count++;
        } else {
            ++it;
        }
    }
    
    SYLAR_LOG_INFO(g_logger) << "Redis connections health check completed: " 
                           << removed_count << " unhealthy connections removed, "
                           << m_conns.size() << "/" << initial_size << " connections remain";
    
    // 如果连接数量不足，补充新连接
    if(m_conns.size() < m_conf.pool_size) {
        size_t need_to_create = m_conf.pool_size - m_conns.size();
        SYLAR_LOG_INFO(g_logger) << "Need to create " << need_to_create << " new Redis connections";
        
        for(size_t i = 0; i < need_to_create; ++i) {
            auto conn = createNewConnection();
            if(conn) {
                m_conns.push_back(conn);
            }
        }
    }
}

bool RedisManager::Init(const std::string& conf_path) {
    std::string path = conf_path;
    if(path.empty()) {
        path = sylar::EnvMgr::GetInstance()->getConfigPath();
    }
    
    SYLAR_LOG_INFO(g_logger) << "Initializing Redis manager with config path: " << path;
    
    try {
        YAML::Node root = YAML::LoadFile(path + "/redis.yml");
        SYLAR_LOG_DEBUG(g_logger) << "Loaded Redis config file, found " << root.size() << " configurations";
        
        for(auto it = root.begin(); it != root.end(); ++it) {
            std::string name = it->first.as<std::string>();
            YAML::Node node = it->second;
            
            SYLAR_LOG_DEBUG(g_logger) << "Processing Redis config: " << name;
            
            RedisConf conf;
            if(conf.loadFromYaml(node)) {
                auto pool = std::make_shared<RedisPool>(conf);
                GetInstance()->addPool(name, pool);
                
                SYLAR_LOG_INFO(g_logger) << "Init Redis pool: " << name 
                                       << " " << conf.host << ":" << conf.port
                                       << " (pool_size: " << conf.pool_size << ")";
            } else {
                SYLAR_LOG_ERROR(g_logger) << "Failed to load Redis config for: " << name;
            }
        }
        
        SYLAR_LOG_INFO(g_logger) << "Redis manager initialized successfully with " 
                               << GetInstance()->m_pools.size() << " pools";
        return true;
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "Load Redis config error: " << e.what();
        return false;
    }
}

RedisManager* RedisManager::GetInstance() {
    static RedisManager instance;
    return &instance;
}

RedisPool::ptr RedisManager::getPool(const std::string& name) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_pools.find(name);
    
    if(it != m_pools.end()) {
        SYLAR_LOG_DEBUG(g_logger) << "Found Redis pool: " << name;
        return it->second;
    } else {
        SYLAR_LOG_WARN(g_logger) << "Redis pool not found: " << name;
        return nullptr;
    }
}

std::shared_ptr<RedisWrapper> RedisManager::getConnection(const std::string& name) {
    SYLAR_LOG_DEBUG(g_logger) << "Getting Redis connection from pool: " << name;
    
    auto pool = getPool(name);
    if(pool) {
        auto conn = pool->getConnection();
        if(conn) {
            SYLAR_LOG_DEBUG(g_logger) << "Successfully got Redis connection from pool: " << name;
        } else {
            SYLAR_LOG_ERROR(g_logger) << "Failed to get Redis connection from pool: " << name;
        }
        return conn;
    }
    
    SYLAR_LOG_ERROR(g_logger) << "Redis pool not found: " << name;
    return nullptr;
}

void RedisManager::addPool(const std::string& name, RedisPool::ptr pool) {
    RWMutexType::WriteLock lock(m_mutex);
    
    if(m_pools.find(name) != m_pools.end()) {
        SYLAR_LOG_WARN(g_logger) << "Redis pool already exists, replacing: " << name;
    } else {
        SYLAR_LOG_DEBUG(g_logger) << "Adding new Redis pool: " << name;
    }
    
    m_pools[name] = pool;
}

} // namespace sylar