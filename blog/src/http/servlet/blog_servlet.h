#ifndef BLOG_SERVER_HTTP_BLOG_SERVLET_H
#define BLOG_SERVER_HTTP_BLOG_SERVLET_H

#include "sylar/http/servlet.h"

namespace blog_server {
namespace http {

class BlogListServlet : public sylar::http::Servlet {
public:
    BlogListServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class BlogListWithUserIdServlet : public sylar::http::Servlet {
public:
    BlogListWithUserIdServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class BlogCreateServlet : public sylar::http::Servlet {
public:
    BlogCreateServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class BlogUpdateServlet : public sylar::http::Servlet {
public:
    BlogUpdateServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class BlogDeleteServlet : public sylar::http::Servlet {
public:
    BlogDeleteServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

class BlogDetailServlet : public sylar::http::Servlet {
public:
    BlogDetailServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                          sylar::http::HttpResponse::ptr response,
                          sylar::http::HttpSession::ptr session) override;
};

} // namespace http
} // namespace blog_server

#endif