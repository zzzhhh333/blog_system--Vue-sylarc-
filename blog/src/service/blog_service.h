#ifndef BLOG_SERVER_SERVICE_BLOG_SERVICE_H
#define BLOG_SERVER_SERVICE_BLOG_SERVICE_H

#include "../model/blog.h"
#include <memory>
#include <vector>

namespace blog_server {
namespace service {

class BlogService {
public:
    typedef std::shared_ptr<BlogService> ptr;
    
    BlogService();
    
    // 博客CRUD操作
    model::Blog createBlog(const model::Blog& blog);
    bool updateBlog(const model::Blog& blog);
    bool deleteBlog(int64_t blog_id, int64_t user_id);
    model::Blog getBlog(int64_t blog_id);
    std::vector<model::Blog> getBlogList(int page = 1, int page_size = 20, int status = 1);
    std::vector<model::Blog> getUserBlogs(int64_t user_id, int page = 1, int page_size = 20);
    
    // 增加阅读量
    void increaseViewCount(int64_t blog_id);

private:
    int64_t generateBlogId();
};

} // namespace service
} // namespace blog_server

#endif