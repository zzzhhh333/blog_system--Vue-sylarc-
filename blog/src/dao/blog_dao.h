// blog_server/dao/blog_dao.h
#ifndef BLOG_SERVER_DAO_BLOG_DAO_H
#define BLOG_SERVER_DAO_BLOG_DAO_H

#include "base_dao.h"
#include "../model/blog.h"
#include "sylar/db/mysql.h"
#include "sylar/log.h"

namespace blog_server {
namespace dao {

/**
 * @brief 博客数据访问对象类
 * 
 * 负责所有与博客表相关的数据库操作，包括：
 * - 博客文章的增删改查
 * - 博客状态管理
 * - 博客搜索和分页查询
 * - 博客统计和视图计数
 */
class BlogDao : public BaseDao<model::Blog> {
public:
    /**
     * @brief 构造函数
     */
    BlogDao();
    
    /**
     * @brief 析构函数
     */
    ~BlogDao() = default;

    // ==================== 基础 CRUD 操作 ====================
    
    /**
     * @brief 创建博客
     * @param blog 博客对象
     * @return 创建成功返回 true，失败返回 false
     */
    bool create(model::Blog& blog) override;
    
    /**
     * @brief 根据ID查找博客
     * @param id 博客ID
     * @return 博客对象
     */
    model::Blog findById(int64_t id) override;
    
    /**
     * @brief 更新博客
     * @param blog 博客对象
     * @return 更新成功返回 true，失败返回 false
     */
    bool update(const model::Blog& blog) override;
    
    /**
     * @brief 删除博客（软删除）
     * @param id 博客ID
     * @return 删除成功返回 true，失败返回 false
     */
    bool remove(int64_t id) override;
    
    // ==================== 查询操作 ====================
    
    /**
     * @brief 查找所有博客
     * @return 博客列表
     */
    std::vector<model::Blog> findAll() override;
    
    /**
     * @brief 根据条件查询博客
     * @param condition 查询条件
     * @return 符合条件的博客列表
     */
    std::vector<model::Blog> findByCondition(const std::string& condition) override;
    
    /**
     * @brief 分页查询博客
     * @param page 页码
     * @param page_size 每页大小
     * @param order_by 排序字段
     * @return 分页后的博客列表
     */
    std::vector<model::Blog> findPage(int page, int page_size, const std::string& order_by = "") override;
    
    // ==================== 统计操作 ====================
    
    /**
     * @brief 统计博客总数
     * @return 博客总数
     */
    int64_t count() override;
    
    /**
     * @brief 根据条件统计博客数量
     * @param condition 统计条件
     * @return 符合条件的博客数量
     */
    int64_t countByCondition(const std::string& condition) override;
    
    // ==================== 博客特定操作 ====================
    
    /**
     * @brief 根据作者ID查找博客
     * @param author_id 作者ID
     * @return 作者的博客列表
     */
    std::vector<model::Blog> findByAuthor(int64_t author_id);
    
    /**
     * @brief 根据状态查找博客
     * @param status 博客状态
     * @return 指定状态的博客列表
     */
    std::vector<model::Blog> findByStatus(int status);
    
    /**
     * @brief 查找已发布的博客（分页）
     * @param page 页码
     * @param page_size 每页大小
     * @return 已发布的博客列表
     */
    std::vector<model::Blog> findPublished(int page = 1, int page_size = 10);
    
    /**
     * @brief 更新博客浏览量
     * @param blog_id 博客ID
     * @param new_count 新的浏览量
     * @return 更新成功返回 true，失败返回 false
     */
    bool updateViewCount(int64_t blog_id, int new_count);
    
    /**
     * @brief 增加博客浏览量
     * @param blog_id 博客ID
     * @return 更新成功返回 true，失败返回 false
     */
    bool increaseViewCount(int64_t blog_id);
    
    /**
     * @brief 发布博客
     * @param blog_id 博客ID
     * @return 发布成功返回 true，失败返回 false
     */
    bool publishBlog(int64_t blog_id);
    
    /**
     * @brief 搜索博客
     * @param keyword 搜索关键词
     * @param page 页码
     * @param page_size 每页大小
     * @return 搜索结果列表
     */
    std::vector<model::Blog> search(const std::string& keyword, int page = 1, int page_size = 10);

private:
    /**
     * @brief 将查询结果转换为博客对象
     * @param result 查询结果
     * @return 博客对象
     */
    model::Blog resultToBlog(sylar::MySQLWrapper::Result::ptr result);
    

private:
    sylar::Logger::ptr logger_;  ///< 日志记录器
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_BLOG_DAO_H