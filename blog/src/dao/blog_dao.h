#ifndef BLOG_SERVER_DAO_BLOG_DAO_H
#define BLOG_SERVER_DAO_BLOG_DAO_H

#include "base_dao.h"
#include "../model/blog.h"
#include "sylar/log.h"
#include <vector>
#include <string>

namespace blog_server {
namespace dao {

/**
 * @brief 博客数据访问对象
 * 
 * 提供博客数据的 CRUD 操作和业务相关查询方法
 */
class BlogDao : public BaseDao<model::Blog, int64_t> {
public:
    using ptr = std::shared_ptr<BlogDao>;
    
    /**
     * @brief 构造函数
     */
    BlogDao();
    
    /**
     * @brief 析构函数
     */
    virtual ~BlogDao();
    
    // ==================== 基础 CRUD 操作实现 ====================
    
    /**
     * @brief 创建博客
     * @param blog 博客对象引用
     * @return 创建成功返回 true，失败返回 false
     */
    virtual bool create(model::Blog& blog) override;
    
    /**
     * @brief 根据ID查找博客
     * @param id 博客ID
     * @param use_cache 是否使用缓存
     * @return 博客对象
     */
    virtual model::Blog findById(int64_t id, bool use_cache = false) override;
    
    /**
     * @brief 更新博客
     * @param blog 博客对象
     * @return 更新成功返回 true，失败返回 false
     */
    virtual bool update(const model::Blog& blog) override;
    
    /**
     * @brief 删除博客（软删除）
     * @param id 博客ID
     * @param soft_delete 是否软删除
     * @return 删除成功返回 true，失败返回 false
     */
    virtual bool remove(int64_t id, bool soft_delete = true) override;
    
    /**
     * @brief 查找所有博客
     * @param order_by 排序字段
     * @return 博客列表
     */
    virtual std::vector<model::Blog> findAll(const std::string& order_by = "") override;
    
    /**
     * @brief 根据条件查询博客
     * @param condition 查询条件
     * @param order_by 排序字段
     * @param limit 限制数量
     * @return 博客列表
     */
    virtual std::vector<model::Blog> findByCondition(const std::string& condition, 
                                                   const std::string& order_by = "", 
                                                   int limit = 0) override;
    
    /**
     * @brief 分页查询博客
     * @param page 页码
     * @param page_size 每页大小
     * @param condition 查询条件
     * @param order_by 排序字段
     * @return 博客列表
     */
    virtual std::vector<model::Blog> findPage(int page, int page_size, 
                                            const std::string& condition = "", 
                                            const std::string& order_by = "") override;
    
    /**
     * @brief 统计博客数量
     * @param condition 统计条件
     * @return 博客数量
     */
    virtual int64_t count(const std::string& condition = "") override;
    
    // ==================== 业务相关查询方法 ====================
    
    /**
     * @brief 根据作者ID查找博客
     * @param author_id 作者ID
     * @param include_deleted 是否包含已删除的博客
     * @return 博客列表
     */
    std::vector<model::Blog> findByAuthor(int64_t author_id, bool include_deleted = false);
    
    /**
     * @brief 根据状态查找博客
     * @param status 博客状态
     * @return 博客列表
     */
    std::vector<model::Blog> findByStatus(int status);
    
    /**
     * @brief 查找已发布的博客（分页）
     * @param page 页码
     * @param page_size 每页大小
     * @return 博客列表
     */
    std::vector<model::Blog> findPublished(int page, int page_size);
    
    /**
     * @brief 更新博客浏览次数
     * @param blog_id 博客ID
     * @param new_count 新的浏览次数
     * @return 更新成功返回 true，失败返回 false
     */
    bool updateViewCount(int64_t blog_id, int new_count);
    
    /**
     * @brief 增加博客浏览次数
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
     * @return 博客列表
     */
    std::vector<model::Blog> search(const std::string& keyword, int page, int page_size);
    
    /**
     * @brief 获取热门博客（按浏览次数排序）
     * @param limit 限制数量
     * @return 博客列表
     */
    std::vector<model::Blog> findPopular(int limit = 10);
    
    /**
     * @brief 获取作者博客统计
     * @param author_id 作者ID
     * @return 统计信息：总博客数、已发布数、总浏览次数
     */
    std::map<std::string, int64_t> getAuthorStats(int64_t author_id);

protected:
    /**
     * @brief 检查博客是否为空
     * @param blog 博客对象
     * @return 为空返回 true，否则返回 false
     */
    virtual bool isEntityEmpty(const model::Blog& blog) const override;
    
    /**
     * @brief 从结果集提取博客对象
     * @param result 查询结果集
     * @return 博客对象
     */
    virtual model::Blog extractEntityFromResult(sylar::db::MySQLResult::ptr result) override;
    
    /**
     * @brief 构建博客对象
     * @param row 结果行
     * @return 博客对象
     */
    model::Blog buildBlogFromRow(const sylar::db::MySQLRow& row);

private:
    sylar::Logger::ptr logger_;  ///< 日志器
};

} // namespace dao
} // namespace blog_server

#endif // BLOG_SERVER_DAO_BLOG_DAO_H