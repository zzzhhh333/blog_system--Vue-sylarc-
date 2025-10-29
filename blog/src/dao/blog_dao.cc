#include "blog_dao.h"
#include "sylar/log.h"
#include <sstream>
#include <iomanip>

namespace blog_server {
namespace dao {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

BlogDao::BlogDao() 
    : BaseDao("blogs") {
    logger_ = g_logger;
    SYLAR_LOG_DEBUG(logger_) << "BlogDao initialized";
    
    // 从连接池获取数据库连接
    if (!acquireConnection()) {
        SYLAR_LOG_ERROR(logger_) << "Failed to acquire database connection";
    }
}

BlogDao::~BlogDao()
{
    // 释放数据库连接
    releaseConnection();

    SYLAR_LOG_DEBUG(logger_) << "BlogDao destroyed";
}

bool BlogDao::create(model::Blog& blog) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "INSERT INTO blogs (title, content, summary, author_id, author_name, "
            "status, view_count, created_at, updated_at, published_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare insert statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        blog.setCreatedAt(now);
        blog.setUpdatedAt(now);
        
        // 如果状态是已发布，设置发布时间
        if (blog.getStatus() == model::Blog::STATUS_PUBLISHED && blog.getPublishedAt() == 0) {
            blog.setPublishedAt(now);
        }
        
        stmt->setString(1, blog.getTitle());
        stmt->setString(2, blog.getContent());
        stmt->setString(3, blog.getSummary());
        stmt->setInt64(4, blog.getAuthorId());
        stmt->setString(5, blog.getAuthorName());
        stmt->setInt32(6, blog.getStatus());
        stmt->setInt32(7, blog.getViewCount());
        stmt->setInt64(8, blog.getCreatedAt());
        stmt->setInt64(9, blog.getUpdatedAt());
        
        if (blog.getPublishedAt() > 0) {
            stmt->setInt64(10, blog.getPublishedAt());
        } else {
            stmt->setNull(10);
        }
        
        bool result = stmt->execute();
        if (!result) {
            SYLAR_LOG_ERROR(logger_) << "Insert blog failed";
            return false;
        }
        
        // 获取自增ID
        blog.setId(connection_->getLastInsertId());
        
        SYLAR_LOG_INFO(logger_) << "Blog created successfully, ID: " << blog.getId() 
                               << ", title: " << blog.getTitle();
        
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Create blog exception: " << e.what();
        return false;
    }
}

model::Blog BlogDao::findById(int64_t id, bool use_cache) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return model::Blog();
    }
    
    try {
        // 如果有缓存实现，可以在这里添加缓存逻辑
        if (use_cache) {
            // TODO: 实现缓存逻辑
        }
        
        auto stmt = connection_->prepareStatement(
            "SELECT id, title, content, summary, author_id, author_name, status, "
            "view_count, created_at, updated_at, published_at "
            "FROM blogs WHERE id = ? AND status != ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare select statement failed";
            return model::Blog();
        }
        
        stmt->setInt64(1, id);
        stmt->setInt32(2, model::Blog::STATUS_DELETED);
        
        auto result = stmt->executeQuery();
        if (!result || !result->next()) {
            SYLAR_LOG_DEBUG(logger_) << "Blog not found, ID: " << id;
            return model::Blog();
        }
        
        SYLAR_LOG_DEBUG(logger_) << "Blog found, ID: " << id;
        return extractEntityFromResult(result);
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find blog by ID exception: " << e.what();
        return model::Blog();
    }
}

bool BlogDao::update(const model::Blog& blog) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE blogs SET title = ?, content = ?, summary = ?, status = ?, "
            "author_name = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        
        stmt->setString(1, blog.getTitle());
        stmt->setString(2, blog.getContent());
        stmt->setString(3, blog.getSummary());
        stmt->setInt32(4, blog.getStatus());
        stmt->setString(5, blog.getAuthorName());
        stmt->setInt64(6, now);
        stmt->setInt64(7, blog.getId());
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update blog failed, blog ID: " << blog.getId();
            return false;
        }
        
        SYLAR_LOG_INFO(logger_) << "Blog updated successfully, ID: " << blog.getId();
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update blog exception: " << e.what();
        return false;
    }
}

bool BlogDao::remove(int64_t id, bool soft_delete) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        if (soft_delete) {
            // 软删除：将状态设置为已删除
            auto stmt = connection_->prepareStatement(
                "UPDATE blogs SET status = ?, updated_at = ? WHERE id = ?"
            );
            
            if (!stmt) {
                SYLAR_LOG_ERROR(logger_) << "Prepare soft delete statement failed";
                return false;
            }
            
            time_t now = time(nullptr);
            stmt->setInt32(1, model::Blog::STATUS_DELETED);
            stmt->setInt64(2, now);
            stmt->setInt64(3, id);
            
            int64_t affected = stmt->executeUpdate();
            if (affected == 0) {
                SYLAR_LOG_ERROR(logger_) << "Soft delete blog failed, ID: " << id;
                return false;
            }
            
            SYLAR_LOG_INFO(logger_) << "Blog soft deleted successfully, ID: " << id;
            return true;
            
        } else {
            // 硬删除：直接从数据库删除
            auto stmt = connection_->prepareStatement("DELETE FROM blogs WHERE id = ?");
            
            if (!stmt) {
                SYLAR_LOG_ERROR(logger_) << "Prepare hard delete statement failed";
                return false;
            }
            
            stmt->setInt64(1, id);
            int64_t affected = stmt->executeUpdate();
            
            if (affected == 0) {
                SYLAR_LOG_ERROR(logger_) << "Hard delete blog failed, ID: " << id;
                return false;
            }
            
            SYLAR_LOG_INFO(logger_) << "Blog hard deleted successfully, ID: " << id;
            return true;
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Remove blog exception: " << e.what();
        return false;
    }
}

std::vector<model::Blog> BlogDao::findAll(const std::string& order_by) {
    std::string condition = "status != " + std::to_string(model::Blog::STATUS_DELETED);
    std::string actual_order_by = order_by.empty() ? "created_at DESC" : order_by;
    
    return findByCondition(condition, actual_order_by);
}

std::vector<model::Blog> BlogDao::findByCondition(const std::string& condition, 
                                                const std::string& order_by, 
                                                int limit) {
    std::vector<model::Blog> blogs;
    
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return blogs;
    }
    
    try {
        std::string sql = "SELECT id, title, content, summary, author_id, author_name, "
                         "status, view_count, created_at, updated_at, published_at "
                         "FROM blogs";
        
        if (!condition.empty()) {
            sql += " WHERE " + condition;
        }
        
        if (!order_by.empty()) {
            sql += " ORDER BY " + order_by;
        }
        
        if (limit > 0) {
            sql += " LIMIT " + std::to_string(limit);
        }
        
        auto result = connection_->executeQuery(sql);
        blogs = extractEntitiesFromResult(result);
        
        SYLAR_LOG_DEBUG(logger_) << "Found " << blogs.size() << " blogs with condition: " 
                                << condition;
        return blogs;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Find blogs by condition exception: " << e.what();
        return blogs;
    }
}

std::vector<model::Blog> BlogDao::findPage(int page, int page_size, 
                                         const std::string& condition, 
                                         const std::string& order_by) {
    std::string actual_condition = condition.empty() ? 
        "status != " + std::to_string(model::Blog::STATUS_DELETED) : condition;
    
    std::string actual_order_by = order_by.empty() ? "created_at DESC" : order_by;
    
    std::string sql = buildPaginationSQL(page, page_size, actual_condition, actual_order_by);
    
    auto result = executeQuery(sql);
    return extractEntitiesFromResult(result);
}

int64_t BlogDao::count(const std::string& condition) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return 0;
    }
    
    try {
        std::string sql = buildCountSQL(condition);
        auto result = connection_->executeQuery(sql);
        
        if (!result || !result->next()) {
            SYLAR_LOG_ERROR(logger_) << "Count blogs failed";
            return 0;
        }
        
        int64_t count = result->getRow().getInt64(0);
        SYLAR_LOG_DEBUG(logger_) << "Blog count: " << count << " with condition: " << condition;
        return count;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Count blogs exception: " << e.what();
        return 0;
    }
}

std::vector<model::Blog> BlogDao::findByAuthor(int64_t author_id, bool include_deleted) {
    std::string condition = "author_id = " + std::to_string(author_id);
    if (!include_deleted) {
        condition += " AND status != " + std::to_string(model::Blog::STATUS_DELETED);
    }
    
    return findByCondition(condition, "created_at DESC");
}

std::vector<model::Blog> BlogDao::findByStatus(int status) {
    std::string condition = "status = " + std::to_string(status);
    return findByCondition(condition, "created_at DESC");
}

std::vector<model::Blog> BlogDao::findPublished(int page, int page_size) {
    std::string condition = "status = " + std::to_string(model::Blog::STATUS_PUBLISHED);
    return findPage(page, page_size, condition, "published_at DESC");
}

bool BlogDao::updateViewCount(int64_t blog_id, int new_count) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE blogs SET view_count = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare update view count statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setInt32(1, new_count);
        stmt->setInt64(2, now);
        stmt->setInt64(3, blog_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Update view count failed, blog ID: " << blog_id;
            return false;
        }
        
        SYLAR_LOG_DEBUG(logger_) << "View count updated for blog: " << blog_id 
                                << ", count: " << new_count;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Update view count exception: " << e.what();
        return false;
    }
}

bool BlogDao::increaseViewCount(int64_t blog_id) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE blogs SET view_count = view_count + 1, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare increase view count statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setInt64(1, now);
        stmt->setInt64(2, blog_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Increase view count failed, blog ID: " << blog_id;
            return false;
        }
        
        SYLAR_LOG_DEBUG(logger_) << "View count increased for blog: " << blog_id;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Increase view count exception: " << e.what();
        return false;
    }
}

bool BlogDao::publishBlog(int64_t blog_id) {
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return false;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "UPDATE blogs SET status = ?, published_at = ?, updated_at = ? WHERE id = ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare publish blog statement failed";
            return false;
        }
        
        time_t now = time(nullptr);
        stmt->setInt32(1, model::Blog::STATUS_PUBLISHED);
        stmt->setInt64(2, now);
        stmt->setInt64(3, now);
        stmt->setInt64(4, blog_id);
        
        int64_t affected = stmt->executeUpdate();
        if (affected == 0) {
            SYLAR_LOG_ERROR(logger_) << "Publish blog failed, blog ID: " << blog_id;
            return false;
        }
        
        SYLAR_LOG_INFO(logger_) << "Blog published successfully, ID: " << blog_id;
        return true;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Publish blog exception: " << e.what();
        return false;
    }
}

std::vector<model::Blog> BlogDao::search(const std::string& keyword, int page, int page_size) {
    if (keyword.empty()) {
        return findPublished(page, page_size);
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT id, title, content, summary, author_id, author_name, status, "
            "view_count, created_at, updated_at, published_at "
            "FROM blogs WHERE status = ? AND (title LIKE ? OR content LIKE ?) "
            "ORDER BY published_at DESC LIMIT ? OFFSET ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare search statement failed";
            return {};
        }
        
        std::string like_keyword = "%" + keyword + "%";
        int offset = (page - 1) * page_size;
        
        stmt->setInt32(1, model::Blog::STATUS_PUBLISHED);
        stmt->setString(2, like_keyword);
        stmt->setString(3, like_keyword);
        stmt->setInt32(4, page_size);
        stmt->setInt32(5, offset);
        
        auto result = stmt->executeQuery();
        std::vector<model::Blog> blogs = extractEntitiesFromResult(result);
        
        SYLAR_LOG_DEBUG(logger_) << "Found " << blogs.size() << " blogs with keyword: " << keyword;
        return blogs;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Search blogs exception: " << e.what();
        return {};
    }
}

std::vector<model::Blog> BlogDao::findPopular(int limit) {
    std::string condition = "status = " + std::to_string(model::Blog::STATUS_PUBLISHED);
    return findByCondition(condition, "view_count DESC", limit);
}

std::map<std::string, int64_t> BlogDao::getAuthorStats(int64_t author_id) {
    std::map<std::string, int64_t> stats;
    
    if (!isConnectionValid()) {
        SYLAR_LOG_ERROR(logger_) << "Database connection not available";
        return stats;
    }
    
    try {
        auto stmt = connection_->prepareStatement(
            "SELECT "
            "COUNT(*) as total_blogs, "
            "SUM(CASE WHEN status = ? THEN 1 ELSE 0 END) as published_blogs, "
            "SUM(view_count) as total_views "
            "FROM blogs WHERE author_id = ? AND status != ?"
        );
        
        if (!stmt) {
            SYLAR_LOG_ERROR(logger_) << "Prepare author stats statement failed";
            return stats;
        }
        
        stmt->setInt32(1, model::Blog::STATUS_PUBLISHED);
        stmt->setInt64(2, author_id);
        stmt->setInt32(3, model::Blog::STATUS_DELETED);
        
        auto result = stmt->executeQuery();
        if (result && result->next()) {
            auto row = result->getRow();
            stats["total_blogs"] = row.getInt64("total_blogs");
            stats["published_blogs"] = row.getInt64("published_blogs");
            stats["total_views"] = row.getInt64("total_views");
        }
        
        return stats;
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Get author stats exception: " << e.what();
        return stats;
    }
}

bool BlogDao::isEntityEmpty(const model::Blog& blog) const {
    return blog.getId() == 0;
}

model::Blog BlogDao::extractEntityFromResult(sylar::db::MySQLResult::ptr result) {
    if (!result) {
        return model::Blog();
    }
    return buildBlogFromRow(result->getRow());
}

model::Blog BlogDao::buildBlogFromRow(const sylar::db::MySQLRow& row) {
    model::Blog blog;
    
    try {
        blog.setId(row.getInt64("id"));
        blog.setTitle(row.getString("title"));
        blog.setContent(row.getString("content"));
        blog.setSummary(row.getString("summary"));
        blog.setAuthorId(row.getInt64("author_id"));
        blog.setAuthorName(row.getString("author_name"));
        blog.setStatus(row.getInt32("status"));
        blog.setViewCount(row.getInt32("view_count"));
        blog.setCreatedAt(row.getInt64("created_at"));
        blog.setUpdatedAt(row.getInt64("updated_at"));
        
        if (!row.isNull("published_at")) {
            blog.setPublishedAt(row.getInt64("published_at"));
        }
        
    } catch (const std::exception& e) {
        SYLAR_LOG_ERROR(logger_) << "Build blog from row failed: " << e.what();
        return model::Blog();
    }
    
    return blog;
}

} // namespace dao
} // namespace blog_server