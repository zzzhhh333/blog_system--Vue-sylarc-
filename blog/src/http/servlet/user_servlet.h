#ifndef BLOG_SERVER_HTTP_USER_SERVLET_H
#define BLOG_SERVER_HTTP_USER_SERVLET_H

#include "sylar/http/servlet.h"

namespace blog_server {
namespace http {

class UserRegisterServlet : public sylar::http::Servlet {
public:
    UserRegisterServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class UserLoginServlet : public sylar::http::Servlet {
public:
    UserLoginServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class UserInfoServlet : public sylar::http::Servlet {
public:
    UserInfoServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class UserProfileServlet : public sylar::http::Servlet {
public:
    UserProfileServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

} // namespace http
} // namespace blog_server

#endif