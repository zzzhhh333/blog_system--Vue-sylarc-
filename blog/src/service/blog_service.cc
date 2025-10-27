#include "blog_service.h"
#include "sylar/db/mysql.h"
#include "sylar/db/redis.h"
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>

namespace blog_server {
namespace service {

// 使用内存存储模拟数据库（实际项目应该使用真实数据库）
static std::map<int64_t, model::Blog> s_blogs;
static std::mutex s_blog_mutex;
static int64_t s_next_blog_id = 1;

BlogService::BlogService() {}

model::Blog BlogService::createBlog(const model::Blog& blog) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    model::Blog new_blog = blog;
    new_blog.id = generateBlogId();
    auto conn=sylar::MySQLMgr::GetInstance()->getConnection(); // 模拟数据库连接获取
    conn->execute("INSERT INTO blogs ()"); // 模拟插入数据库操作
    s_blogs[new_blog.id] = new_blog;
    
    return new_blog;
}

bool BlogService::updateBlog(const model::Blog& blog) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    auto it = s_blogs.find(blog.id);
    if (it == s_blogs.end()) {
        return false;
    }
    
    // 更新博客信息
    model::Blog& existing_blog = it->second;
    existing_blog.title = blog.title;
    existing_blog.content = blog.content;
    existing_blog.summary = blog.summary;
    existing_blog.status = blog.status;
    existing_blog.updated_at = time(nullptr);
    
    if (blog.status == 1 && existing_blog.published_at == 0) {
        existing_blog.published_at = time(nullptr);
    }
    
    return true;
}

bool BlogService::deleteBlog(int64_t blog_id, int64_t user_id) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    auto it = s_blogs.find(blog_id);
    if (it == s_blogs.end() || it->second.author_id != user_id) {
        return false;
    }
    
    s_blogs.erase(it);
    return true;
}

model::Blog BlogService::getBlog(int64_t blog_id) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    auto it = s_blogs.find(blog_id);
    if (it == s_blogs.end()) {
        return model::Blog(); // 返回空对象
    }
    
    return it->second;
}

std::vector<model::Blog> BlogService::getBlogList(int page, int page_size, int status) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    std::vector<model::Blog> result;
    for (const auto& pair : s_blogs) {
        if (status == -1 || pair.second.status == status) {
            result.push_back(pair.second);
        }
    }
    
    // 按创建时间倒序排序
    std::sort(result.begin(), result.end(), [](const model::Blog& a, const model::Blog& b) {
        return a.created_at > b.created_at;
    });
    
    // 分页
    int start = (page - 1) * page_size;
    if (start >= (int)result.size()) {
        return std::vector<model::Blog>();
    }
    
    int end = std::min(start + page_size, (int)result.size());
    return std::vector<model::Blog>(result.begin() + start, result.begin() + end);
}

std::vector<model::Blog> BlogService::getUserBlogs(int64_t user_id, int page, int page_size) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    std::vector<model::Blog> result;
    for (const auto& pair : s_blogs) {
        if (pair.second.author_id == user_id) {
            result.push_back(pair.second);
        }
    }
    
    // 按创建时间倒序排序
    std::sort(result.begin(), result.end(), [](const model::Blog& a, const model::Blog& b) {
        return a.created_at > b.created_at;
    });
    
    // 分页
    int start = (page - 1) * page_size;
    if (start >= (int)result.size()) {
        return std::vector<model::Blog>();
    }
    
    int end = std::min(start + page_size, (int)result.size());
    return std::vector<model::Blog>(result.begin() + start, result.begin() + end);
}

void BlogService::increaseViewCount(int64_t blog_id) {
    std::lock_guard<std::mutex> lock(s_blog_mutex);
    
    auto it = s_blogs.find(blog_id);
    if (it != s_blogs.end()) {
        it->second.view_count++;
    }
}

int64_t BlogService::generateBlogId() {
    return s_next_blog_id++;
}

} // namespace service
} // namespace blog_server