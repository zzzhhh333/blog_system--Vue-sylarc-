cmake_minimum_required(VERSION 3.0)
project(sylar-from-scratch)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror -g")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)

option(BUILD_TEST "ON for complile test" ON)

find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    sylar/log.cpp
    sylar/util.cpp
    sylar/mutex.cc
    sylar/env.cc
    sylar/config.cc
    sylar/thread.cc
    sylar/fiber.cc
    sylar/scheduler.cc
    sylar/iomanager.cc
    sylar/timer.cc
    sylar/fd_manager.cc
    sylar/hook.cc
    sylar/address.cc 
    sylar/socket.cc 
    sylar/bytearray.cc 
    sylar/tcp_server.cc 
    sylar/http/http-parser/http_parser.c 
    sylar/http/http.cc
    sylar/http/http_parser.cc 
    sylar/stream.cc 
    sylar/streams/socket_stream.cc
    sylar/http/http_session.cc 
    sylar/http/servlet.cc
    sylar/http/http_server.cc 
    sylar/uri.cc 
    sylar/http/http_connection.cc 
    sylar/daemon.cc 
    )

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar)

set(LIBS
    sylar
    pthread
    dl
    yaml-cpp
    jsoncpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(BUILD_TEST)
sylar_add_executable(test_log "tests/test_log.cpp" sylar "${LIBS}")
sylar_add_executable(test_util "tests/test_util.cpp" sylar "${LIBS}")
sylar_add_executable(test_env "tests/test_env.cc" sylar "${LIBS}")
sylar_add_executable(test_config "tests/test_config.cc" sylar "${LIBS}")
sylar_add_executable(test_thread "tests/test_thread.cc" sylar "${LIBS}")
sylar_add_executable(test_fiber "tests/test_fiber.cc" sylar "${LIBS}")
sylar_add_executable(test_fiber2 "tests/test_fiber2.cc" sylar "${LIBS}")
sylar_add_executable(test_scheduler "tests/test_scheduler.cc" sylar "${LIBS}")
sylar_add_executable(test_iomanager "tests/test_iomanager.cc" sylar "${LIBS}")
sylar_add_executable(test_timer "tests/test_timer.cc" sylar "${LIBS}")
sylar_add_executable(test_hook "tests/test_hook.cc" sylar "${LIBS}")
sylar_add_executable(test_address "tests/test_address.cc" sylar "${LIBS}")
sylar_add_executable(test_socket_tcp_server "tests/test_socket_tcp_server.cc" sylar "${LIBS}")
sylar_add_executable(test_socket_tcp_client "tests/test_socket_tcp_client.cc" sylar "${LIBS}")
sylar_add_executable(test_bytearray "tests/test_bytearray.cc" sylar "${LIBS}")
sylar_add_executable(test_tcp_server "tests/test_tcp_server.cc" sylar "${LIBS}")
sylar_add_executable(test_http "tests/test_http.cc" sylar "${LIBS}")
sylar_add_executable(test_http_parser "tests/test_http_parser.cc" sylar "${LIBS}")
sylar_add_executable(test_http_server "tests/test_http_server.cc" sylar "${LIBS}")
sylar_add_executable(test_uri "tests/test_uri.cc" sylar "${LIBS}")
sylar_add_executable(test_http_connection "tests/test_http_connection.cc" sylar "${LIBS}")
sylar_add_executable(test_daemon "tests/test_daemon.cc" sylar "${LIBS}")


endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 添加博客服务器构建选项
option(BUILD_BLOG_SERVER "Build blog server" ON)

if(BUILD_BLOG_SERVER)
    message(STATUS "Building blog server...")
    
    # 博客服务器的源文件（使用绝对路径）
    set(BLOG_SRC
        ${CMAKE_SOURCE_DIR}/blog/src/main.cc
        ${CMAKE_SOURCE_DIR}/blog/src/core/server.cc
        ${CMAKE_SOURCE_DIR}/blog/src/http/servlet/blog_servlet.cc
        ${CMAKE_SOURCE_DIR}/blog/src/http/servlet/user_servlet.cc
        ${CMAKE_SOURCE_DIR}/blog/src/service/blog_service.cc
        ${CMAKE_SOURCE_DIR}/blog/src/service/user_service.cc
        ${CMAKE_SOURCE_DIR}/blog/src/util/json_util.cc
        ${CMAKE_SOURCE_DIR}/blog/src/util/crypto_util.cc
        ${CMAKE_SOURCE_DIR}/blog/src/util/auth_util.cc
    )
    
    # 包含博客项目的头文件
    include_directories(${CMAKE_SOURCE_DIR}/../blog/src)
    
    # 添加博客服务器可执行文件
    add_executable(blog_server ${BLOG_SRC})
    
    # 链接库
    target_link_libraries(blog_server ${LIBS})
endif()