#ifndef BLOG_SERVER_SERVICE_USER_SERVICE_H
#define BLOG_SERVER_SERVICE_USER_SERVICE_H

#include "../model/user.h"
#include "../dao/user_dao.h"
#include <memory>
#include <string>

namespace blog_server {
namespace service {

class UserService {
public:
    typedef std::shared_ptr<UserService> ptr;
    
    UserService();
    
    // 用户注册登录
    model::User registerUser(const std::string& username, const std::string& password, 
                            const std::string& nickname, const std::string& email);
    model::User login(const std::string& username, const std::string& password);
    
    // 用户信息管理
    model::User getUserById(int64_t user_id);
    model::User getUserByUsername(const std::string& username);
    bool updateUser(const model::User& user);
    
    // 验证用户
    bool validateUser(int64_t user_id, const std::string& password);
    
    // 重置用户密码
    bool resetPassword(int64_t user_id, const std::string& new_password);

    // 更改用户密码
    bool updatePassword(int64_t user_id, const std::string& old_password, const std::string& new_password);


private:
    std::shared_ptr<dao::UserDao> user_dao_;

    int64_t generateUserId();
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hashed);
    void setUserDao(std::shared_ptr<dao::UserDao> user_dao);

};

} // namespace service
} // namespace blog_server

#endif