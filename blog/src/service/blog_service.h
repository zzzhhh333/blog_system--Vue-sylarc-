// blog_server/service/blog_service.h
#ifndef BLOG_SERVER_SERVICE_BLOG_SERVICE_H
#define BLOG_SERVER_SERVICE_BLOG_SERVICE_H

#include "../dao/blog_dao.h"
#include "../model/blog.h"
#include <vector>
#include <string>

namespace blog_server {
namespace service {

/**
 * @brief 博客服务类
 * 
 * 提供博客相关的业务逻辑，包括：
 * - 博客创建、编辑、删除
 * - 博客发布和状态管理
 * - 博客搜索和分页查询
 * - 博客统计和视图计数
 */
class BlogService {
public:
    /**
     * @brief 构造函数
     */
    BlogService();
    
    /**
     * @brief 设置博客数据访问对象
     * @param blog_dao 博客 DAO 指针
     */
    void setBlogDao(std::shared_ptr<dao::BlogDao> blog_dao);
    
    /**
     * @brief 创建博客
     * @param blog 博客对象
     * @return 创建成功的博客对象
     */
    model::Blog createBlog(const model::Blog& blog);
    
    /**
     * @brief 更新博客
     * @param blog 博客对象
     * @return 更新成功返回 true，失败返回 false
     */
    bool updateBlog(const model::Blog& blog);
    
    /**
     * @brief 删除博客
     * @param blog_id 博客ID
     * @param user_id 用户ID（用于权限验证）
     * @return 删除成功返回 true，失败返回 false
     */
    bool deleteBlog(int64_t blog_id, int64_t user_id);
    
    /**
     * @brief 获取博客
     * @param blog_id 博客ID
     * @return 博客对象
     */
    model::Blog getBlog(int64_t blog_id);
    
    /**
     * @brief 获取博客列表
     * @param page 页码
     * @param page_size 每页大小
     * @param status 博客状态（-1表示所有状态）
     * @return 博客列表
     */
    std::vector<model::Blog> getBlogList(int page, int page_size, int status = -1);
    
    /**
     * @brief 获取用户博客
     * @param user_id 用户ID
     * @param page 页码
     * @param page_size 每页大小
     * @return 用户博客列表
     */
    std::vector<model::Blog> getUserBlogs(int64_t user_id, int page, int page_size);
    
    /**
     * @brief 增加博客浏览量
     * @param blog_id 博客ID
     * @return 增加成功返回 true，失败返回 false
     */
    bool increaseViewCount(int64_t blog_id);
    
    /**
     * @brief 发布博客
     * @param blog_id 博客ID
     * @param user_id 用户ID（用于权限验证）
     * @return 发布成功返回 true，失败返回 false
     */
    bool publishBlog(int64_t blog_id, int64_t user_id);
    
    /**
     * @brief 搜索博客
     * @param keyword 搜索关键词
     * @param page 页码
     * @param page_size 每页大小
     * @return 搜索结果列表
     */
    std::vector<model::Blog> searchBlogs(const std::string& keyword, int page, int page_size);
    
    /**
     * @brief 验证博客所有权
     * @param blog_id 博客ID
     * @param user_id 用户ID
     * @return 用户拥有该博客返回 true，否则返回 false
     */
    bool validateOwnership(int64_t blog_id, int64_t user_id);

private:
    std::shared_ptr<dao::BlogDao> blog_dao_;  ///< 博客数据访问对象
};

} // namespace service
} // namespace blog_server

#endif // BLOG_SERVER_SERVICE_BLOG_SERVICE_H