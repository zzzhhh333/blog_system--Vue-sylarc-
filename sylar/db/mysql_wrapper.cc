// mysql_wrapper.cpp
#include "mysql_wrapper.h"
#include <cstring>
#include "sylar/log.h"  // 添加Sylar日志头文件

namespace sylar {

// 定义MySQL专用的日志器
static sylar::Logger::ptr g_mysql_logger = SYLAR_LOG_NAME("root");

/**
 * @brief 构造函数
 * 
 * 初始化MySQL连接句柄，但不建立实际连接。
 */
MySQLWrapper::MySQLWrapper() 
    : mysql_(nullptr), result_(nullptr), row_(nullptr), num_fields_(0), fields_(nullptr) {
    // 初始化MySQL连接结构体
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Failed to initialize MySQL";
    } else {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL wrapper initialized successfully";
    }
}

/**
 * @brief 析构函数
 * 
 * 自动断开连接并清理资源。
 */
MySQLWrapper::~MySQLWrapper() {
    disconnect();
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL wrapper destroyed";
    }
}

/**
 * @brief 连接到MySQL数据库
 */
bool MySQLWrapper::connect(const std::string& host, const std::string& user,
                          const std::string& password, const std::string& database,
                          unsigned int port, const std::string& charset) {
    SYLAR_LOG_INFO(g_mysql_logger) << "Connecting to MySQL: " << host << ":" << port 
                                  << ", database: " << database << ", user: " << user;
    
    // 建立到MySQL服务器的连接
    if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(), 
                           password.c_str(), database.c_str(), port, nullptr, 0)) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "MySQL connection failed: " << mysql_error(mysql_);
        return false;
    }
    
    // 设置字符集
    if (mysql_set_character_set(mysql_, charset.c_str()) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Set charset failed: " << mysql_error(mysql_);
        return false;
    }
    
    SYLAR_LOG_INFO(g_mysql_logger) << "MySQL connected successfully, charset: " << charset;
    return true;
}

/**
 * @brief 断开数据库连接
 */
void MySQLWrapper::disconnect() {
    clearResult();
    SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL disconnected";
}

/**
 * @brief 检查连接状态
 */
bool MySQLWrapper::isConnected() {
    bool connected = mysql_ && mysql_ping(mysql_) == 0;
    if (!connected) {
        SYLAR_LOG_WARN(g_mysql_logger) << "MySQL connection check failed";
    }
    return connected;
}

/**
 * @brief 执行SQL语句（用于INSERT/UPDATE/DELETE等）
 */
bool MySQLWrapper::execute(const std::string& sql) {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Execute SQL: " << sql;
    
    clearResult();
    bool success = mysql_query(mysql_, sql.c_str()) == 0;
    
    if (!success) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Execute SQL failed: " << mysql_error(mysql_) 
                                       << ", SQL: " << sql;
    } else {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Execute SQL success, affected rows: " << getAffectedRows();
    }
    
    return success;
}

/**
 * @brief 执行查询语句（用于SELECT）
 */
bool MySQLWrapper::query(const std::string& sql) {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Query SQL: " << sql;
    
    clearResult();
    
    // 执行SQL查询
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Query SQL failed: " << mysql_error(mysql_)
                                       << ", SQL: " << sql;
        return false;
    }
    
    // 获取并存储结果集
    result_ = mysql_store_result(mysql_);
    if (!result_) {
        // 对于没有结果集的查询（如UPDATE等），这不算错误
        if (mysql_field_count(mysql_) == 0) {
            SYLAR_LOG_DEBUG(g_mysql_logger) << "Query has no result set";
            return true;
        } else {
            SYLAR_LOG_ERROR(g_mysql_logger) << "Store result failed: " << mysql_error(mysql_);
            return false;
        }
    }
    
    // 获取结果集的列信息和列数
    num_fields_ = mysql_num_fields(result_);
    fields_ = mysql_fetch_fields(result_);
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Query success, fields: " << num_fields_;
    return true;
}

/**
 * @brief 获取受影响的行数
 */
int MySQLWrapper::getAffectedRows() {
    return mysql_affected_rows(mysql_);
}

/**
 * @brief 获取最后插入的ID
 */
long long MySQLWrapper::getLastInsertId() {
    long long id = mysql_insert_id(mysql_);
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Last insert ID: " << id;
    return id;
}

/**
 * @brief 移动到结果集的下一行
 */
bool MySQLWrapper::next() {
    if (!result_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "No result set available for next()";
        return false;
    }
    
    row_ = mysql_fetch_row(result_);
    bool has_next = row_ != nullptr;
    
    if (!has_next) {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "No more rows in result set";
    }
    
    return has_next;
}

/**
 * @brief 根据列名获取整数值
 */
int MySQLWrapper::getInt(const std::string& column) {
    int index = getColumnIndex(column);
    if (index == -1) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
        return 0;
    }
    return getInt(index);
}

/**
 * @brief 根据列索引获取整数值
 */
int MySQLWrapper::getInt(int index) {
    if (!row_ || index < 0 || index >= (int)num_fields_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return 0;
    }
    
    int value = row_[index] ? std::stoi(row_[index]) : 0;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Get int value: " << value << " at index: " << index;
    return value;
}

/**
 * @brief 根据列名获取字符串值
 */
std::string MySQLWrapper::getString(const std::string& column) {
    int index = getColumnIndex(column);
    if (index == -1) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
        return "";
    }
    return getString(index);
}

/**
 * @brief 根据列索引获取字符串值
 */
std::string MySQLWrapper::getString(int index) {
    if (!row_ || index < 0 || index >= (int)num_fields_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return "";
    }
    
    std::string value = row_[index] ? row_[index] : "";
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Get string value: " << value << " at index: " << index;
    return value;
}

/**
 * @brief 根据列名获取浮点数值
 */
double MySQLWrapper::getDouble(const std::string& column) {
    int index = getColumnIndex(column);
    if (index == -1) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
        return 0.0;
    }
    return getDouble(index);
}

/**
 * @brief 根据列索引获取浮点数值
 */
double MySQLWrapper::getDouble(int index) {
    if (!row_ || index < 0 || index >= (int)num_fields_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return 0.0;
    }
    
    double value = row_[index] ? std::stod(row_[index]) : 0.0;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Get double value: " << value << " at index: " << index;
    return value;
}

/**
 * @brief 检查指定列是否为NULL
 */
bool MySQLWrapper::isNull(const std::string& column) {
    int index = getColumnIndex(column);
    if (index == -1) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
        return true;
    }
    return isNull(index);
}

/**
 * @brief 检查指定列是否为NULL
 */
bool MySQLWrapper::isNull(int index) {
    if (!row_ || index < 0 || index >= (int)num_fields_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return true;
    }
    
    bool is_null = row_[index] == nullptr;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Column at index " << index << " is null: " << is_null;
    return is_null;
}

/**
 * @brief 开始事务
 */
bool MySQLWrapper::beginTransaction() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Begin transaction";
    bool success = execute("START TRANSACTION");
    if (success) {
        SYLAR_LOG_INFO(g_mysql_logger) << "Transaction started";
    } else {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Begin transaction failed";
    }
    return success;
}

/**
 * @brief 提交事务
 */
bool MySQLWrapper::commit() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Commit transaction";
    bool success = execute("COMMIT");
    if (success) {
        SYLAR_LOG_INFO(g_mysql_logger) << "Transaction committed";
    } else {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Commit transaction failed";
    }
    return success;
}

/**
 * @brief 回滚事务
 */
bool MySQLWrapper::rollback() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Rollback transaction";
    bool success = execute("ROLLBACK");
    if (success) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Transaction rolled back";
    } else {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Rollback transaction failed";
    }
    return success;
}

/**
 * @brief 创建预处理语句
 */
std::shared_ptr<MySQLWrapper::PreparedStatement> MySQLWrapper::prepare(const std::string& sql) {
     return std::make_shared<PreparedStatement>(this, sql);
}

/**
 * @brief 获取错误信息
 */
std::string MySQLWrapper::getError() {
    std::string error = mysql_error(mysql_);
    if (!error.empty()) {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL error: " << error;
    }
    return error;
}

/**
 * @brief 获取错误代码
 */
int MySQLWrapper::getErrorCode() {
    int code = mysql_errno(mysql_);
    if (code != 0) {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL error code: " << code;
    }
    return code;
}

/**
 * @brief 清理结果集资源
 */
void MySQLWrapper::clearResult() {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
        row_ = nullptr;
        num_fields_ = 0;
        fields_ = nullptr;
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Result set cleared";
    }
}

/**
 * @brief 根据列名获取列索引
 */
int MySQLWrapper::getColumnIndex(const std::string& column) {
    if (!fields_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "No fields available";
        return -1;
    }
    
    // 遍历所有列，查找匹配的列名
    for (unsigned int i = 0; i < num_fields_; i++) {
        if (std::string(fields_[i].name) == column) {
            SYLAR_LOG_DEBUG(g_mysql_logger) << "Found column '" << column << "' at index " << i;
            return i;
        }
    }
    
    SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
    return -1;
}

// ==================== PreparedStatement 实现 ====================

/**
 * @brief 预处理语句构造函数
 */
MySQLWrapper::PreparedStatement::PreparedStatement(MySQLWrapper* db, const std::string& sql) 
    : stmt_(nullptr), params_(nullptr) {
    // 初始化预处理语句
    stmt_ = mysql_stmt_init(db->mysql_);
    if (stmt_) {
        // 准备SQL语句
        if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.length()) != 0) {
            SYLAR_LOG_ERROR(g_mysql_logger) << "Prepare statement failed: " << mysql_stmt_error(stmt_);
            mysql_stmt_close(stmt_);
            stmt_ = nullptr;
            return;
        }
        
        // 获取参数数量并初始化参数绑定数组
        int param_count = mysql_stmt_param_count(stmt_);
        if (param_count > 0) {
            params_ = new MYSQL_BIND[param_count];
            memset(params_, 0, sizeof(MYSQL_BIND) * param_count);
            SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement created with " << param_count << " parameters";
        } else {
            SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement created with no parameters";
        }
    } else {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Init prepared statement failed";
    }
}

/**
 * @brief 预处理语句析构函数
 */
MySQLWrapper::PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement destroyed";
    }
    if (params_) {
        delete[] params_;
    }
    // 清理字符串缓冲区
    for (char* buf : stringBuffers_) {
        delete[] buf;
    }
}

/**
 * @brief 执行预处理语句
 */
bool MySQLWrapper::PreparedStatement::execute() {
    if (!stmt_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Prepared statement is null";
        return false;
    }
    
    // 绑定参数并执行
    if (params_) {
        if (mysql_stmt_bind_param(stmt_, params_) != 0) {
            SYLAR_LOG_ERROR(g_mysql_logger) << "Bind parameters failed: " << mysql_stmt_error(stmt_);
            return false;
        }
    }
    
    bool success = mysql_stmt_execute(stmt_) == 0;
    if (success) {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement executed successfully";
    } else {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Execute prepared statement failed: " << mysql_stmt_error(stmt_);
    }
    
    return success;
}

/**
 * @brief 执行查询预处理语句
 */
bool MySQLWrapper::PreparedStatement::query() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Execute prepared statement query";
    return execute();
}

/**
 * @brief 绑定整型参数
 */
void MySQLWrapper::PreparedStatement::setInt(int paramIndex, int value) {
    if (!params_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "No parameters allocated for setInt";
        return;
    }
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_LONG;
    params_[paramIndex].buffer = &value;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Set int parameter at index " << paramIndex << ": " << value;
}

/**
 * @brief 绑定浮点型参数
 */
void MySQLWrapper::PreparedStatement::setDouble(int paramIndex, double value) {
    if (!params_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "No parameters allocated for setDouble";
        return;
    }
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_DOUBLE;
    params_[paramIndex].buffer = &value;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Set double parameter at index " << paramIndex << ": " << value;
}

/**
 * @brief 绑定字符串参数
 */
void MySQLWrapper::PreparedStatement::setString(int paramIndex, const std::string& value) {
    if (!params_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "No parameters allocated for setString";
        return;
    }
    
    // 创建字符串缓冲区并复制数据
    char* buffer = new char[value.size() + 1];
    strcpy(buffer, value.c_str());
    stringBuffers_.push_back(buffer);
    
    // 设置参数绑定信息
    params_[paramIndex].buffer_type = MYSQL_TYPE_STRING;
    params_[paramIndex].buffer = buffer;
    params_[paramIndex].buffer_length = value.size();
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Set string parameter at index " << paramIndex 
                                   << ": " << value << " (length: " << value.size() << ")";
}

/**
 * @brief 绑定NULL参数
 */
void MySQLWrapper::PreparedStatement::setNull(int paramIndex) {
    if (!params_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "No parameters allocated for setNull";
        return;
    }
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_NULL;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Set null parameter at index " << paramIndex;
}

} // namespace sylar