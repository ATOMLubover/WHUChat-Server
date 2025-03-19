# C++ 部分后端共享信息 README

## 后端大致架构

与前端直接交互的是 C++ boost::asio 和 boost::beast 开发的 ` GateServer `  
` GateServer ` 处理前端 Web 发送的 **HTTP 请求**，并决定如何调用其他服务器，获得其他服务器的处理结果，并最后综合返回 **HTTP 回复**给 Web

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

在上面这个例子中，访问 *server-domain*:*port*/login-test时会自动返回 index.html，而其他依赖文件也会在浏览器自动构造 GET 请求后获得

---

## ` GateServer `的监听地址

本地运行时如下  
IPv4 ：**0.0.0.0**  
port ：**8080**

---

## HTTP POST 请求反馈错误码一览

> 如果返回体是 JSON 格式，则会以 "error" 字段存储

| int32值 | 名称 | 描述 |
| :- | :- | :- |
| 0 | Success | gate server 正常处理请求 |