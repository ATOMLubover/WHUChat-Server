# C++ 部分后端共享信息 README

## 后端大致架构

与前端直接交互的是 C++ boost::asio 和 boost::beast 开发的 ` GateServer `  
` GateServer ` 处理前端 Web 发送的 **HTTP 请求**，并决定如何调用其他服务器，获得其他服务器的处理结果，并最后综合返回 **HTTP 回复**给 Web

---

## 后端存储前端文件的组织形式

前端发送到 ` GateServer `的 URL 根路由应当与后端存储目录的格式相同，方便后端抓取对应资源文件返回  
以下为一个例子

> *project-root* 是一个功能模块的名字，如 “login-dialog”

```text
project-root/
│
├── public/                  # 静态资源目录（不会被 webpack 处理）
│   ├── index.html           # 项目入口 HTML 文件
│   └── favicon.ico          # 网站图标
│
├── src/                     # 项目源代码目录
│   ├── assets/              # 静态资源（图片、字体等，会被 webpack 处理）
│   │   ├── images/          # 图片资源
│   │   └── fonts/           # 字体资源
│   │
│   ├── components/          # 公共组件
│   │   ├── Header.vue       # 头部组件
│   │   └── Footer.vue       # 底部组件
│   │
│   ├── views/               # 页面级组件（路由页面）
│   │   ├── Home.vue         # 首页
│   │   └── About.vue        # 关于页面
│   │
│   ├── router/              # 路由配置
│   │   └── index.js         # 路由配置文件
│   │
│   ├── store/               # Vuex 状态管理（可选）
│   │   └── index.js         # Vuex 配置文件
│   │
│   ├── styles/              # 全局样式
│   │   ├── main.scss        # 全局样式文件
│   │   └── variables.scss   # 样式变量
│   │
│   ├── utils/               # 工具函数
│   │   └── request.js       # 封装请求工具
│   │
│   ├── App.vue              # 根组件
│   ├── main.js              # 项目入口文件
│   └── api/                 # API 接口封装
│       └── user.js          # 用户相关接口
│
├── .env                     # 环境变量配置文件
├── .env.development         # 开发环境变量
├── .env.production          # 生产环境变量
│
├── package.json             # 项目依赖和脚本配置
├── vite.config.js           # Vite 配置文件（如果使用 Vite）
└── README.md                # 项目说明文档
```

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