# C++ 部分后端共享信息 README

## 后端大致架构

与前端直接交互的是 C++ boost::asio 和 boost::beast 开发的 ` GateServer `  
` GateServer ` 处理前端 Web 发送的 **HTTP 请求**，并决定如何调用其他服务器，获得其他服务器的处理结果，并最后综合返回 **HTTP 回复** 给 Web

` GateServer ` 会通过 gRPC 调用 Node.js 开发的 ` VrfGrpcServer ` 进行注册账号时的验证码发送

---

## 后端存储前端文件的组织形式

前端发送到 ` GateServer `的 URL 根路由应当与后端存储目录的格式相同，方便后端抓取对应资源文件返回  
以下为一个例子

> *project-root* 是一个功能模块的名字，如 “login-test”

```text
project-root/
│
├── scripts/         # js 所在文件夹
│   └── main.js
│
├── styles/          # css 所在文件夹
│   └── main.css
│
└── index.html       # index（当使用默认路由时自动返回）
```

在上面这个例子中，访问 *server-domain*:*port*/login-test/ 时会自动返回 index.html（只针对/进行了自动重定向），而其他依赖文件也会在浏览器自动构造 GET 请求后获得（**必须要注意：此处 URL 最后的 “/” 不可以省略，否则将会导致无法正确找到对应文件**）

---

## ` GateServer ` 的监听地址

本地运行时如下  
IPv4 ：**0.0.0.0**  
port ：**8080**

---

## ` GateServer ` 已实现的接口

**GET**

| URI | 参数描述 | 返回值 | 功能 |
| :- | :- | :- | :- |
| /favicon.ico | 无参数 | application/octet-stream | 返回 favicon.ico 网站图标 |

## HTTP POST 请求反馈错误码一览

> 如果返回体是 JSON 格式，则会以 "error" 字段存储

| int32值 | 名称 | 描述 |
| :- | :- | :- |
| 0 | Success | 正常处理请求 |
| 101 | ErrorRedis *trs* | ` VrfGrpcServer ` 调用 Redis 出现错误 |
| 102 | ErrorSend *trs* | ` VrfGrpcServer ` 未能成功发送验证邮件  |
| 103 | ErrorException *trs* | ` VrfGrpcServer ` 未定义异常 |
| 1001 | ErrorServerNotResponding | ` GateServer ` 未收到其他服务器的响应 |
| 1002 | ErrorGrpc | ` GateServer ` 调用 gRPC 出现错误 |
| 1003 | ErrorJson | ` GateServer ` 处理前端传输的  JSON 出现错误 |