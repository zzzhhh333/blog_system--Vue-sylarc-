// mysql_wrapper.cpp
#include "mysql_wrapper.h"
#include "sylar/log.h"
#include "sylar/util.h"
#include <cstring>
#include <algorithm>

namespace sylar {

static Logger::ptr g_mysql_logger = SYLAR_LOG_NAME("system");

// ==================== MySQLWrapper 实现 ====================

MySQLWrapper::MySQLWrapper() {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Failed to initialize MySQL";
    } else {
        // 设置连接选项
        mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        mysql_options(mysql_, MYSQL_OPT_RECONNECT, &config_.reconnect);
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL wrapper initialized";
    }
}

MySQLWrapper::~MySQLWrapper() {
    disconnect();
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL wrapper destroyed";
    }
}

bool MySQLWrapper::connect(const Config& config) {
    config_ = config;
    return connect(config.host, config.user, config.password, 
                  config.database, config.port, config.charset);
}

bool MySQLWrapper::connect(const std::string& host, const std::string& user,
                          const std::string& password, const std::string& database,
                          unsigned int port, const std::string& charset) {
    SYLAR_LOG_INFO(g_mysql_logger) << "Connecting to MySQL: " << host << ":" << port 
                                  << ", database: " << database;
    
    // 设置连接超时
    unsigned int timeout = config_.timeout;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    // 建立连接
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
    
    SYLAR_LOG_INFO(g_mysql_logger) << "MySQL connected successfully";
    return true;
}

void MySQLWrapper::disconnect() {
    if (mysql_ && isConnected()) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        SYLAR_LOG_DEBUG(g_mysql_logger) << "MySQL disconnected";
    }
}

bool MySQLWrapper::isConnected() {
    return mysql_ && mysql_ping(mysql_) == 0;
}

bool MySQLWrapper::ping() {
    if (!mysql_) return false;
    
    if (mysql_ping(mysql_) != 0) {
        SYLAR_LOG_WARN(g_mysql_logger) << "MySQL ping failed, reconnecting...";
        return reconnect();
    }
    return true;
}

bool MySQLWrapper::execute(const std::string& sql) {
    if (!ping()) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Database not connected";
        return false;
    }
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Execute SQL: " << sql;
    
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Execute SQL failed: " << mysql_error(mysql_)
                                       << ", SQL: " << sql;
        return false;
    }
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Execute SQL success, affected rows: " << getAffectedRows();
    return true;
}

MySQLWrapper::Result::ptr MySQLWrapper::query(const std::string& sql) {
    if (!ping()) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Database not connected";
        return nullptr;
    }
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Query SQL: " << sql;
    
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Query SQL failed: " << mysql_error(mysql_)
                                       << ", SQL: " << sql;
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) {
        if (mysql_field_count(mysql_) == 0) {
            SYLAR_LOG_DEBUG(g_mysql_logger) << "Query has no result set";
            return nullptr;
        } else {
            SYLAR_LOG_ERROR(g_mysql_logger) << "Store result failed: " << mysql_error(mysql_);
            return nullptr;
        }
    }
    
    return std::make_shared<Result>(result);
}

MySQLWrapper::PreparedStatement::ptr MySQLWrapper::prepare(const std::string& sql) {
    if (!ping()) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Database not connected";
        return nullptr;
    }
    
    return std::make_shared<PreparedStatement>(this, sql);
}

bool MySQLWrapper::beginTransaction() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Begin transaction";
    if (execute("START TRANSACTION")) {
        in_transaction_ = true;
        SYLAR_LOG_INFO(g_mysql_logger) << "Transaction started";
        return true;
    }
    SYLAR_LOG_ERROR(g_mysql_logger) << "Begin transaction failed";
    return false;
}

bool MySQLWrapper::commit() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Commit transaction";
    if (execute("COMMIT")) {
        in_transaction_ = false;
        SYLAR_LOG_INFO(g_mysql_logger) << "Transaction committed";
        return true;
    }
    SYLAR_LOG_ERROR(g_mysql_logger) << "Commit transaction failed";
    return false;
}

bool MySQLWrapper::rollback() {
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Rollback transaction";
    if (execute("ROLLBACK")) {
        in_transaction_ = false;
        SYLAR_LOG_WARN(g_mysql_logger) << "Transaction rolled back";
        return true;
    }
    SYLAR_LOG_ERROR(g_mysql_logger) << "Rollback transaction failed";
    return false;
}

int MySQLWrapper::getAffectedRows() {
    return mysql_affected_rows(mysql_);
}

int64_t MySQLWrapper::getLastInsertId() {
    return mysql_insert_id(mysql_);
}

std::string MySQLWrapper::getError() {
    return mysql_error(mysql_);
}

int MySQLWrapper::getErrorCode() {
    return mysql_errno(mysql_);
}

void MySQLWrapper::clearError() {
    // MySQL 自动清理错误信息
}

bool MySQLWrapper::reconnect() {
    SYLAR_LOG_INFO(g_mysql_logger) << "Attempting to reconnect to MySQL";
    disconnect();
    
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Failed to reinitialize MySQL";
        return false;
    }
    
    return connect(config_);
}

// ==================== Result 实现 ====================

MySQLWrapper::Result::Result(MYSQL_RES* res) : result_(res) {
    if (result_) {
        field_count_ = mysql_num_fields(result_);
        fields_ = mysql_fetch_fields(result_);
        row_count_ = mysql_num_rows(result_);
        
        // 构建字段名到索引的映射
        for (int i = 0; i < field_count_; i++) {
            field_index_map_[fields_[i].name] = i;
        }
        
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Result set created, fields: " 
                                       << field_count_ << ", rows: " << row_count_;
    }
}

MySQLWrapper::Result::~Result() {
    if (result_) {
        mysql_free_result(result_);
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Result set destroyed";
    }
}

bool MySQLWrapper::Result::next() {
    if (!result_) return false;
    
    row_ = mysql_fetch_row(result_);
    if (row_) {
        lengths_ = mysql_fetch_lengths(result_);
    }
    return row_ != nullptr;
}

int MySQLWrapper::Result::getColumnIndex(const std::string& column) {
    auto it = field_index_map_.find(column);
    if (it != field_index_map_.end()) {
        return it->second;
    }
    
    SYLAR_LOG_WARN(g_mysql_logger) << "Column not found: " << column;
    return -1;
}

int MySQLWrapper::Result::getInt(const std::string& column) {
    int index = getColumnIndex(column);
    return index >= 0 ? getInt(index) : 0;
}

int MySQLWrapper::Result::getInt(int index) {
    if (!row_ || index < 0 || index >= field_count_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return 0;
    }
    return row_[index] ? std::stoi(row_[index]) : 0;
}

int64_t MySQLWrapper::Result::getInt64(const std::string& column) {
    int index = getColumnIndex(column);
    return index >= 0 ? getInt64(index) : 0;
}

int64_t MySQLWrapper::Result::getInt64(int index) {
    if (!row_ || index < 0 || index >= field_count_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return 0;
    }
    return row_[index] ? std::stoll(row_[index]) : 0;
}

std::string MySQLWrapper::Result::getString(const std::string& column) {
    int index = getColumnIndex(column);
    return index >= 0 ? getString(index) : "";
}

std::string MySQLWrapper::Result::getString(int index) {
    if (!row_ || index < 0 || index >= field_count_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return "";
    }
    return row_[index] ? std::string(row_[index], lengths_[index]) : "";
}

double MySQLWrapper::Result::getDouble(const std::string& column) {
    int index = getColumnIndex(column);
    return index >= 0 ? getDouble(index) : 0.0;
}

double MySQLWrapper::Result::getDouble(int index) {
    if (!row_ || index < 0 || index >= field_count_) {
        SYLAR_LOG_WARN(g_mysql_logger) << "Invalid column index: " << index;
        return 0.0;
    }
    return row_[index] ? std::stod(row_[index]) : 0.0;
}

bool MySQLWrapper::Result::isNull(const std::string& column) {
    int index = getColumnIndex(column);
    return index >= 0 ? isNull(index) : true;
}

bool MySQLWrapper::Result::isNull(int index) {
    if (!row_ || index < 0 || index >= field_count_) {
        return true;
    }
    return row_[index] == nullptr;
}

std::string MySQLWrapper::Result::getFieldName(int index) const {
    if (index < 0 || index >= field_count_ || !fields_) {
        return "";
    }
    return fields_[index].name;
}

// ==================== PreparedStatement 实现 ====================

MySQLWrapper::PreparedStatement::PreparedStatement(MySQLWrapper* db, const std::string& sql) 
    : db_(db) {
    stmt_ = mysql_stmt_init(db_->mysql_);
    if (!stmt_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Init prepared statement failed";
        return;
    }
    
    if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.length()) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Prepare statement failed: " << mysql_stmt_error(stmt_);
        mysql_stmt_close(stmt_);
        stmt_ = nullptr;
        return;
    }
    
    param_count_ = mysql_stmt_param_count(stmt_);
    if (param_count_ > 0) {
        params_ = new MYSQL_BIND[param_count_];
        memset(params_, 0, sizeof(MYSQL_BIND) * param_count_);
    }
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement created: " << sql 
                                   << " (params: " << param_count_ << ")";
}

MySQLWrapper::PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
    }
    if (params_) {
        delete[] params_;
    }
    for (char* buf : buffers_) {
        delete[] buf;
    }
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement destroyed";
}

bool MySQLWrapper::PreparedStatement::execute() {
    if (!stmt_) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Prepared statement is null";
        return false;
    }
    
    if (param_count_ > 0 && mysql_stmt_bind_param(stmt_, params_) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Bind parameters failed: " << mysql_stmt_error(stmt_);
        return false;
    }
    
    if (mysql_stmt_execute(stmt_) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Execute prepared statement failed: " << mysql_stmt_error(stmt_);
        return false;
    }
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement executed successfully";
    return true;
}

MySQLWrapper::Result::ptr MySQLWrapper::PreparedStatement::query() {
    if (!execute()) {
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt_);
    if (!result) {
        SYLAR_LOG_DEBUG(g_mysql_logger) << "Prepared statement has no result set";
        return nullptr;
    }
    
    // 存储结果
    if (mysql_stmt_store_result(stmt_) != 0) {
        SYLAR_LOG_ERROR(g_mysql_logger) << "Store result failed: " << mysql_stmt_error(stmt_);
        mysql_free_result(result);
        return nullptr;
    }
    
    return std::make_shared<Result>(result);
}

void MySQLWrapper::PreparedStatement::setInt(int paramIndex, int value) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_LONG;
    params_[paramIndex].buffer = new int(value);
    buffers_.push_back(reinterpret_cast<char*>(params_[paramIndex].buffer));
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind int: " << value << " at index " << paramIndex;
}

void MySQLWrapper::PreparedStatement::setInt64(int paramIndex, int64_t value) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_LONGLONG;
    params_[paramIndex].buffer = new int64_t(value);
    buffers_.push_back(reinterpret_cast<char*>(params_[paramIndex].buffer));
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind int64: " << value << " at index " << paramIndex;
}

void MySQLWrapper::PreparedStatement::setDouble(int paramIndex, double value) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_DOUBLE;
    params_[paramIndex].buffer = new double(value);
    buffers_.push_back(reinterpret_cast<char*>(params_[paramIndex].buffer));
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind double: " << value << " at index " << paramIndex;
}

void MySQLWrapper::PreparedStatement::setString(int paramIndex, const std::string& value) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    char* buffer = new char[value.size() + 1];
    strcpy(buffer, value.c_str());
    buffers_.push_back(buffer);
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_STRING;
    params_[paramIndex].buffer = buffer;
    params_[paramIndex].buffer_length = value.size();
    params_[paramIndex].length = new unsigned long(value.size());
    buffers_.push_back(reinterpret_cast<char*>(params_[paramIndex].length));
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind string: " << value << " at index " << paramIndex;
}

void MySQLWrapper::PreparedStatement::setNull(int paramIndex) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_NULL;
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind null at index " << paramIndex;
}

void MySQLWrapper::PreparedStatement::setBlob(int paramIndex, const void* data, unsigned long length) {
    if (!params_ || paramIndex < 0 || paramIndex >= param_count_) return;
    
    char* buffer = new char[length];
    memcpy(buffer, data, length);
    buffers_.push_back(buffer);
    
    params_[paramIndex].buffer_type = MYSQL_TYPE_BLOB;
    params_[paramIndex].buffer = buffer;
    params_[paramIndex].buffer_length = length;
    params_[paramIndex].length = new unsigned long(length);
    buffers_.push_back(reinterpret_cast<char*>(params_[paramIndex].length));
    
    SYLAR_LOG_DEBUG(g_mysql_logger) << "Bind blob, length: " << length << " at index " << paramIndex;
}

int MySQLWrapper::PreparedStatement::getAffectedRows() {
    return stmt_ ? mysql_stmt_affected_rows(stmt_) : 0;
}

int64_t MySQLWrapper::PreparedStatement::getLastInsertId() {
    return stmt_ ? mysql_stmt_insert_id(stmt_) : 0;
}

} // namespace sylar