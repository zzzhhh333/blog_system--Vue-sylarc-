// blog_server/dao/blog_dao.cpp
#include "blog_dao.h"
#include "sylar/db/mysql.h"
#include <sstream>
#include <iomanip>

namespace blog_server {
namespace dao {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

BlogDao::BlogDao() {
    logger_ = g_logger;
    SYLAR_LOG_DEBUG(logger_) << "BlogDao initialized";
    db_=sylar::MySQLMgr::GetInstance()->getConnection();
}

bool BlogDao::create(model::Blog& blog) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare(
        "INSERT INTO blogs (title, content, summary, author_id, author_name, status, view_count, created_at, updated_at, published_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare insert statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    blog.created_at = now;
    blog.updated_at = now;
    
    // 如果状态是已发布，设置发布时间
    if (blog.status == 1 && blog.published_at == 0) {
        blog.published_at = now;
    }
    
    stmt->setString(1, blog.title);
    stmt->setString(2, blog.content);
    stmt->setString(3, blog.summary);
    stmt->setInt64(4, blog.author_id);
    stmt->setString(5, blog.author_name);
    stmt->setInt(6, blog.status);
    stmt->setInt(7, blog.view_count);
    stmt->setInt64(8, blog.created_at);
    stmt->setInt64(9, blog.updated_at);
    
    if (blog.published_at > 0) {
        stmt->setInt64(10, blog.published_at);
    } else {
        stmt->setNull(10);
    }
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Insert blog failed: " << db_->getError();
        return false;
    }
    
    blog.id =stmt->getLastInsertId();
    SYLAR_LOG_INFO(logger_) << "Blog created successfully, ID: " << blog.id 
                           << ", title: " << blog.title;
    
    return true;
}

model::Blog BlogDao::findById(int64_t id) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return model::Blog();
    }
    
    auto stmt = db_->prepare(
        "SELECT id, title, content, summary, author_id, author_name, status, view_count, created_at, updated_at, published_at "
        "FROM blogs WHERE id = ? AND status != 2"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare select statement failed";
        return model::Blog();
    }
    
    stmt->setInt64(1, id);
    auto result = stmt->query();
    
    if (!result || !result->next()) {
        SYLAR_LOG_DEBUG(logger_) << "Blog not found, ID: " << id;
        return model::Blog();
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Blog found, ID: " << id;
    return resultToBlog(result);
}

bool BlogDao::update(const model::Blog& blog) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare(
        "UPDATE blogs SET title = ?, content = ?, summary = ?, status = ?, updated_at = ? "
        "WHERE id = ?"
    );
    
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare update statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    
    stmt->setString(1, blog.title);
    stmt->setString(2, blog.content);
    stmt->setString(3, blog.summary);
    stmt->setInt(4, blog.status);
    stmt->setInt64(5, now);
    stmt->setInt64(6, blog.id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Update blog failed: " << db_->getError()
                                << ", blog ID: " << blog.id;
        return false;
    }
    
    SYLAR_LOG_INFO(logger_) << "Blog updated successfully, ID: " << blog.id;
    return true;
}

bool BlogDao::remove(int64_t id) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    // 软删除：将状态设置为2（已删除）
    auto stmt = db_->prepare("UPDATE blogs SET status = 2, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare delete statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setInt64(1, now);
    stmt->setInt64(2, id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Delete blog failed: " << db_->getError()
                                << ", blog ID: " << id;
        return false;
    }
    
    SYLAR_LOG_INFO(logger_) << "Blog deleted successfully, ID: " << id;
    return true;
}

std::vector<model::Blog> BlogDao::findAll() {
    return findByCondition("status != 2 ORDER BY created_at DESC");
}

std::vector<model::Blog> BlogDao::findByCondition(const std::string& condition) {
    std::vector<model::Blog> blogs;
    
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return blogs;
    }
    
    std::string sql = "SELECT id, title, content, summary, author_id, author_name, status, view_count, created_at, updated_at, published_at "
                      "FROM blogs";
    if (!condition.empty()) {
        sql += " WHERE " + condition;
    }
    
    auto result = db_->query(sql);
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Query blogs failed: " << db_->getError();
        return blogs;
    }
    
    while (result->next()) {
        blogs.push_back(resultToBlog(result));
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Found " << blogs.size() << " blogs with condition: " << condition;
    return blogs;
}

std::vector<model::Blog> BlogDao::findPage(int page, int page_size, const std::string& order_by) {
    std::vector<model::Blog> blogs;
    
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return blogs;
    }
    
    if (page < 1) page = 1;
    if (page_size < 1) page_size = 10;
    
    std::string sql = "SELECT id, title, content, summary, author_id, author_name, status, view_count, created_at, updated_at, published_at "
                      "FROM blogs WHERE status != 2";
    
    if (!order_by.empty()) {
        sql += " ORDER BY " + order_by;
    } else {
        sql += " ORDER BY created_at DESC";
    }
    
    sql += " LIMIT ? OFFSET ?";
    
    auto stmt = db_->prepare(sql);
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare paged query statement failed";
        return blogs;
    }
    
    int offset = (page - 1) * page_size;
    stmt->setInt(1, page_size);
    stmt->setInt(2, offset);
    
    auto result = stmt->query();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Execute paged query failed: " << db_->getError();
        return blogs;
    }
    
    while (result->next()) {
        blogs.push_back(resultToBlog(result));
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Found " << blogs.size() << " blogs on page " << page 
                            << " (size: " << page_size << ")";
    return blogs;
}

int64_t BlogDao::count() {
    return countByCondition("status != 2");
}

int64_t BlogDao::countByCondition(const std::string& condition) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return 0;
    }
    
    std::string sql = "SELECT COUNT(*) FROM blogs";
    if (!condition.empty()) {
        sql += " WHERE " + condition;
    }
    
    auto result = db_->query(sql);
    if (!result || !result->next()) {
        SYLAR_LOG_ERROR(logger_) << "Count blogs failed: " << db_->getError();
        return 0;
    }
    
    int64_t count = result->getInt64(0);
    SYLAR_LOG_DEBUG(logger_) << "Blog count: " << count << " with condition: " << condition;
    return count;
}

std::vector<model::Blog> BlogDao::findByAuthor(int64_t author_id) {
    return findByCondition("author_id = " + std::to_string(author_id) + " AND status != 2 ORDER BY created_at DESC");
}

std::vector<model::Blog> BlogDao::findByStatus(int status) {
    return findByCondition("status = " + std::to_string(status) + " ORDER BY created_at DESC");
}

std::vector<model::Blog> BlogDao::findPublished(int page, int page_size) {
    return findPage(page, page_size, "published_at DESC");
}

bool BlogDao::updateViewCount(int64_t blog_id, int new_count) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE blogs SET view_count = ?, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare update view count statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setInt(1, new_count);
    stmt->setInt64(2, now);
    stmt->setInt64(3, blog_id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Update view count failed: " << db_->getError()
                                << ", blog ID: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_DEBUG(logger_) << "View count updated for blog: " << blog_id << ", count: " << new_count;
    return true;
}

bool BlogDao::increaseViewCount(int64_t blog_id) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE blogs SET view_count = view_count + 1, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare increase view count statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setInt64(1, now);
    stmt->setInt64(2, blog_id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Increase view count failed: " << db_->getError()
                                << ", blog ID: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_DEBUG(logger_) << "View count increased for blog: " << blog_id;
    return true;
}

bool BlogDao::publishBlog(int64_t blog_id) {
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE blogs SET status = 1, published_at = ?, updated_at = ? WHERE id = ?");
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare publish blog statement failed";
        return false;
    }
    
    time_t now = time(nullptr);
    stmt->setInt64(1, now);
    stmt->setInt64(2, now);
    stmt->setInt64(3, blog_id);
    
    auto result = stmt->execute();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Publish blog failed: " << db_->getError()
                                << ", blog ID: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_INFO(logger_) << "Blog published successfully, ID: " << blog_id;
    return true;
}

std::vector<model::Blog> BlogDao::search(const std::string& keyword, int page, int page_size) {
    std::vector<model::Blog> blogs;
    
    if (!db_) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not set";
        return blogs;
    }
    
    if (keyword.empty()) {
        return findPublished(page, page_size);
    }
    
    std::string sql = "SELECT id, title, content, summary, author_id, author_name, status, view_count, created_at, updated_at, published_at "
                      "FROM blogs WHERE status = 1 AND (title LIKE ? OR content LIKE ?) "
                      "ORDER BY published_at DESC LIMIT ? OFFSET ?";
    
    auto stmt = db_->prepare(sql);
    if (!stmt) {
        SYLAR_LOG_ERROR(logger_) << "Prepare search statement failed";
        return blogs;
    }
    
    std::string like_keyword = "%" + keyword + "%";
    int offset = (page - 1) * page_size;
    
    stmt->setString(1, like_keyword);
    stmt->setString(2, like_keyword);
    stmt->setInt(3, page_size);
    stmt->setInt(4, offset);
    
    auto result = stmt->query();
    if (!result) {
        SYLAR_LOG_ERROR(logger_) << "Execute search failed: " << db_->getError();
        return blogs;
    }
    
    while (result->next()) {
        blogs.push_back(resultToBlog(result));
    }
    
    SYLAR_LOG_DEBUG(logger_) << "Found " << blogs.size() << " blogs with keyword: " << keyword;
    return blogs;
}

model::Blog BlogDao::resultToBlog(sylar::MySQLWrapper::Result::ptr result) {
    model::Blog blog;
    
    try {
        blog.id = result->getInt64("id");
        blog.title = result->getString("title");
        blog.content = result->getString("content");
        blog.summary = result->getString("summary");
        blog.author_id = result->getInt64("author_id");
        blog.author_name = result->getString("author_name");
        blog.status = result->getInt("status");
        blog.view_count = result->getInt("view_count");
        blog.created_at = result->getInt64("created_at");
        blog.updated_at = result->getInt64("updated_at");
        
        if (!result->isNull("published_at")) {
            blog.published_at = result->getInt64("published_at");
        }
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Convert result to blog failed: " << e.what();
        return model::Blog();
    }
    
    return blog;
}



} // namespace dao
} // namespace blog_server