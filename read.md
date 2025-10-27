# 博客系统 API 文档

## 项目概述

基于 **Sylar C++ 高性能服务器框架** 开发的博客系统，提供完整的博客管理和用户认证功能。

## 系统架构
博客系统架构
┌─────────────────┐ HTTP/JSON ┌──────────────────┐
│ 前端客户端 │ ←─────────────→ │ Blog Server │
│ (Web/App/Postman)│ │ (C++ Sylar框架) │
└─────────────────┘ └──────────────────┘
│
┌───────┼───────┐
│ │
┌─────▼─────┐ ┌─────▼─────┐
│ 业务逻辑层 │ │ 数据访问层 │
│ Service │ │ Model │
└───────────┘ └───────────┘

## 技术栈

- **框架**: Sylar C++ 高性能服务器框架
- **数据格式**: JSON
- **认证**: Token 认证
- **存储**: 内存存储
- **并发**: 线程安全设计

## API 接口文档

### 用户认证模块

#### 1. 用户注册

**接口**: `POST /api/users/register`

**功能**: 新用户注册

**请求体**:
```json
{
  "username": "string, 3-20字符",
  "password": "string, 最少6字符", 
  "nickname": "string, 可选",
  "email": "string, 可选"
}
```
**响应**:
```json
{
  "code": 0,
  "message": "注册成功",
  "data": {
    "token": "认证令牌",
    "user": {
      "id": 1,
      "username": "testuser",
      "nickname": "测试用户",
      "email": "test@example.com"
    }
  }
}
```


#### 2. 用户登录
**接口**: POST /api/users/login

**功能**: 用户登录

**请求体**:
```json
{
  "username": "用户名",
  "password": "密码"
}
```
**响应**:
```json
{
  "code": 0,
  "message": "注册成功",
  "data": {
    "token": "认证令牌",
    "user": {
      "id": 1,
      "username": "testuser",
      "nickname": "测试用户",
      "email": "test@example.com"
    }
  }
}
```

#### 3. 获取用户信息
**接口**: GET /api/users/info

**功能**: 获取当前用户或指定用户信息

**参数**:

user_id: 可选，指定用户ID

**Headers**:

Authorization: Bearer <token> 或 Cookie: token=<token>

**响应**:
```json
{
  "code": 0,
  "message": "success",
  "data": {
    "id": 1,
    "username": "testuser",
    "nickname": "测试用户",
    "email": "test@example.com",
    "avatar": "",
    "bio": "",
    "created_at": "2024-01-01 00:00:00"
  }
}
```

#### 4. 更新用户资料
**接口**: POST /api/users/profile

**功能**: 更新当前用户资料

**Headers**: 需要认证

**请求体**:
```json
{
  "nickname": "新昵称",
  "email": "新邮箱",
  "avatar": "头像URL",
  "bio": "个人简介"
}
```

**响应**:
```json
{
  "code": 0,
  "message": "资料更新成功",
  "data": {
    "id": 1,
    "username": "testuser",
    "nickname": "新昵称",
    "email": "new@example.com",
    "avatar": "https://example.com/avatar.jpg",
    "bio": "这是我的个人简介"
  }
}
```

### 博客管理模块
#### 1. 获取博客列表
**接口**: GET /api/blogs

**功能**: 分页获取博客列表

**参数**:

page: 页码，默认1

page_size: 每页数量，默认20

status: 博客状态，1=已发布（默认），-1=所有状态

**响应**:
```json
{
  "code": 0,
  "message": "success",
  "data": [
    {
      "id": 1,
      "title": "博客标题",
      "summary": "博客摘要",
      "author_name": "作者名",
      "created_at": "2024-01-01 00:00:00",
      "view_count": 100
    }
  ]
}
```
#### 2. 创建博客
**接口**: POST /api/blogs/create

**功能**: 创建新博客

**Headers**: 需要认证

**请求体**:
```json
{
  "title": "博客标题",
  "content": "博客内容",
  "summary": "摘要，可选",
  "status": "状态，0=草稿，1=发布"
}
```

**响应**:
```json
{
  "code": 0,
  "message": "博客创建成功",
  "data": {
    "id": 1,
    "title": "博客标题",
    "content": "博客内容",
    "author_name": "当前用户",
    "status": 1,
    "created_at": "2024-01-01 00:00:00"
  }
}
```
#### 3. 更新博客
**接口**: POST /api/blogs/update

**功能**: 更新博客内容

**Headers**: 需要认证

**请求体**:
```json
{
  "id": "博客ID",
  "title": "新标题",
  "content": "新内容",
  "summary": "新摘要",
  "status": "新状态"
}
```
**响应**:
```json
{
  "code": 0,
  "message": "博客更新成功"
}
```

#### 4. 删除博客
**接口**: GET /api/blogs/delete

**功能**: 删除博客

**Headers**: 需要认证

**参数**:

id: 博客ID

**响应**:
```json
{
  "code": 0,
  "message": "博客删除成功"
}
```
#### 5. 获取博客详情
**接口**: GET /api/blogs/detail

**功能**: 获取博客详细信息

**参数**:

id: 博客ID

**响应**:

```json
{
  "code": 0,
  "message": "success",
  "data": {
    "id": 1,
    "title": "博客标题",
    "content": "完整的博客内容",
    "summary": "博客摘要",
    "author_id": 1,
    "author_name": "作者名",
    "status": 1,
    "view_count": 150,
    "created_at": "2024-01-01 00:00:00",
    "updated_at": "2024-01-02 00:00:00"
  }
}
```

### 认证机制
#### Token 认证
**生成**: 用户登录/注册时生成

**存储**: 服务器内存存储（可扩展为Redis）

**传递方式**:

HTTP Header: Authorization: Bearer <token>

Cookie: token=<token>

**有效期**: 当前为永久有效（生产环境应设置过期时间）

#### 权限控制
博客创建、更新、删除需要登录认证

用户只能修改自己的博客

博客列表和详情可公开访问
