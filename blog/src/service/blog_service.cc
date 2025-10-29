// blog_server/service/blog_service.cpp
#include "blog_service.h"
#include "../dao/blog_dao.h"
#include <algorithm>

namespace blog_server {
namespace service {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

BlogService::BlogService() {
    SYLAR_LOG_DEBUG(g_logger) << "BlogService initialized";
    blog_dao_=std::make_shared<dao::BlogDao>();
}

void BlogService::setBlogDao(std::shared_ptr<dao::BlogDao> blog_dao) {
    blog_dao_ = blog_dao;
    SYLAR_LOG_DEBUG(g_logger) << "BlogDAO set for BlogService";
}

model::Blog BlogService::createBlog(const model::Blog& blog) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return model::Blog();
    }
    
    // 验证输入参数
    if (blog.getTitle().empty() || blog.getContent().empty()) {
        SYLAR_LOG_ERROR(g_logger) << "Blog title or content is empty";
        return model::Blog();
    }
    
    if (blog.getAuthorId() <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid author ID: " << blog.getAuthorId();
        return model::Blog();
    }
    
    // 创建博客对象
    model::Blog new_blog = blog;
    new_blog.setViewCount(0);
    
    // 保存博客
    if (!blog_dao_->create(new_blog)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to create blog: " << blog.getTitle();
        return model::Blog();
    }
    
    SYLAR_LOG_INFO(g_logger) << "Blog created successfully: " << new_blog.getTitle() << " (ID: " << new_blog.getId() << ")";
    return new_blog;
}

bool BlogService::updateBlog(const model::Blog& blog) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return false;
    }
    
    int64_t id=blog.getId();

    if (id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid blog ID: " << id;
        return false;
    }
    
    // 获取现有博客信息
    model::Blog existing_blog = blog_dao_->findById(id);
    if (existing_blog.getId() == 0) {
        SYLAR_LOG_ERROR(g_logger) << "Blog not found for update: " << id;
        return false;
    }
    
    // 创建更新对象
    model::Blog update_blog = existing_blog;
    update_blog.setTitle(blog.getTitle());
    update_blog.setContent(blog.getContent());
    update_blog.setSummary(blog.getSummary());
    update_blog.setStatus(blog.getStatus());
    
    // 如果状态从未发布变为已发布，设置发布时间
    if (existing_blog.getStatus() != 1 && blog.getStatus() == 1 && update_blog.getPublishedAt() == 0) {
        update_blog.setPublishedAt(time(nullptr));
    }
    
    // 执行更新
    if (!blog_dao_->update(update_blog)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to update blog: " << id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Blog updated successfully: " << id;
    return true;
}

bool BlogService::deleteBlog(int64_t blog_id, int64_t user_id) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return false;
    }
    
    if (blog_id <= 0 || user_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid blog ID or user ID";
        return false;
    }
    
    // 验证博客所有权
    if (!validateOwnership(blog_id, user_id)) {
        SYLAR_LOG_ERROR(g_logger) << "User " << user_id << " does not own blog " << blog_id;
        return false;
    }
    
    // 执行删除
    if (!blog_dao_->remove(blog_id)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to delete blog: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Blog deleted successfully: " << blog_id;
    return true;
}

model::Blog BlogService::getBlog(int64_t blog_id) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return model::Blog();
    }
    
    if (blog_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid blog ID: " << blog_id;
        return model::Blog();
    }
    
    model::Blog blog = blog_dao_->findById(blog_id);
    if (blog.getId() == 0) {
        SYLAR_LOG_DEBUG(g_logger) << "Blog not found: " << blog_id;
    }
    
    return blog;
}

std::vector<model::Blog> BlogService::getBlogList(int page, int page_size, int status) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return std::vector<model::Blog>();
    }
    
    if (page < 1) page = 1;
    if (page_size < 1) page_size = 10;
    
    std::vector<model::Blog> blogs;
    
    if (status == -1) {
        // 获取所有博客
        blogs = blog_dao_->findPage(page, page_size);
    } else {
        // 获取指定状态的博客
        blogs = blog_dao_->findByStatus(status);
        
        // 手动分页
        int start = (page - 1) * page_size;
        if (start >= (int)blogs.size()) {
            return std::vector<model::Blog>();
        }
        
        int end = std::min(start + page_size, (int)blogs.size());
        blogs = std::vector<model::Blog>(blogs.begin() + start, blogs.begin() + end);
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "Found " << blogs.size() << " blogs on page " << page;
    return blogs;
}

std::vector<model::Blog> BlogService::getUserBlogs(int64_t user_id, int page, int page_size) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return std::vector<model::Blog>();
    }
    
    if (user_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid user ID: " << user_id;
        return std::vector<model::Blog>();
    }
    
    if (page < 1) page = 1;
    if (page_size < 1) page_size = 10;
    
    std::vector<model::Blog> blogs = blog_dao_->findByAuthor(user_id);
    
    // 手动分页
    int start = (page - 1) * page_size;
    if (start >= (int)blogs.size()) {
        return std::vector<model::Blog>();
    }
    
    int end = std::min(start + page_size, (int)blogs.size());
    std::vector<model::Blog> result(blogs.begin() + start, blogs.begin() + end);
    
    SYLAR_LOG_DEBUG(g_logger) << "Found " << result.size() << " blogs for user " << user_id << " on page " << page;
    return result;
}

bool BlogService::increaseViewCount(int64_t blog_id) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return false;
    }
    
    if (blog_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid blog ID: " << blog_id;
        return false;
    }
    
    if (!blog_dao_->increaseViewCount(blog_id)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to increase view count for blog: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "View count increased for blog: " << blog_id;
    return true;
}

bool BlogService::publishBlog(int64_t blog_id, int64_t user_id) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return false;
    }
    
    if (blog_id <= 0 || user_id <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid blog ID or user ID";
        return false;
    }
    
    // 验证博客所有权
    if (!validateOwnership(blog_id, user_id)) {
        SYLAR_LOG_ERROR(g_logger) << "User " << user_id << " does not own blog " << blog_id;
        return false;
    }
    
    if (!blog_dao_->publishBlog(blog_id)) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to publish blog: " << blog_id;
        return false;
    }
    
    SYLAR_LOG_INFO(g_logger) << "Blog published successfully: " << blog_id;
    return true;
}

std::vector<model::Blog> BlogService::searchBlogs(const std::string& keyword, int page, int page_size) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return std::vector<model::Blog>();
    }
    
    if (page < 1) page = 1;
    if (page_size < 1) page_size = 10;
    
    std::vector<model::Blog> blogs = blog_dao_->search(keyword, page, page_size);
    
    SYLAR_LOG_DEBUG(g_logger) << "Found " << blogs.size() << " blogs with keyword: " << keyword;
    return blogs;
}

bool BlogService::validateOwnership(int64_t blog_id, int64_t user_id) {
    if (!blog_dao_) {
        SYLAR_LOG_ERROR(g_logger) << "BlogDAO not set";
        return false;
    }
    
    model::Blog blog = blog_dao_->findById(blog_id);
    if (blog.getId() == 0) {
        SYLAR_LOG_ERROR(g_logger) << "Blog not found: " << blog_id;
        return false;
    }
    
    bool owns = (blog.getAuthorId() == user_id);
    SYLAR_LOG_DEBUG(g_logger) << "User " << user_id << " owns blog " << blog_id << ": " << owns;
    return owns;
}

} // namespace service
} // namespace blog_server