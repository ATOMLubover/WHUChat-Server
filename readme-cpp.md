# C++ 部分后端共享信息 README

## 后端大致架构

与前端直接交互的是 C++ boost::asio 和 boost::beast 开发的 ` GateServer ` 和 ` ChatServer `  
` GateServer ` 处理前端 Web 发送的 **HTTP 请求**，并决定如何调用其他服务器，获得其他服务器的处理结果，并最后综合返回 **HTTP 响应** 给 Web  
` ChatServer ` 处理前端 Web 发送的 **HTTP 请求**，并且决定是返回 **HTTP 响应** 给前端，还是升级为 **Websocket 连接**  

后端服务器调用关系：

` GateServer ` 通过 gRPC 调用 Node.js 开发的 ` VerifiServer ` 进行注册账号时的验证码发送  
` GateServer ` 通过 gRPC 调用 C++ 开发的 ` StatusServer ` 进行客户端分配，
随后指示客户端转接到 ` ChatServer `  

---

## 后端代理前端文件的组织形式

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

在上面这个例子中，访问 *server-domain*:*port*/login-test 时会自动返回 index.html（只针对/进行了自动重定向），而其他依赖文件也会在浏览器自动构造 GET 请求后获得

---

## ` GateServer ` 的监听地址

本地运行时如下  
IPv4 ：**0.0.0.0**  
port ：**8080**

---

## ` GateServer ` 的接口
  
> 斜体字为注释  

> 所有 text 类型的返回值默认是 UTF-8 编码

**GET**

`/favicon.ico`

- 请求参数  

    无

- 返回值（application/octet-stream）

    返回网站图标二进制文件

- 补充  

    无

`/login`

- 请求参数  

    无

- 返回值（text/html）  

    返回登录页面HTML文档

- 补充  

    无

`/register`

- 请求参数  

    无

- 返回值（text/html）  

    返回注册页面HTML文档

- 补充  

    无

**POST**

`/api/v1/login`

- 请求参数（JSON）

    | 参数名       | 类型   | 必填 | 说明                | 示例值               |
    | :----------- | :----- | :--- | :-------------------| :------------------- |
    | `email`      | string | 是   | 用户注册邮箱        | `"user@domain.com"`  |
    | `password`   | string | 是   | 用户密码（明文）    | `"P@ssw0rd"`         |

- 返回值（JSON）

    | 参数名       | 类型   | 说明                      | 示例值                     |
    | :----------- | :----- | :------------------------ | :------------------------- |
    | `uuid`       | int | 用户唯一标识              | `2233`              |
    | `token`      | string | 会话凭证                  | `"1145-dd19"`            |
    | `host`       | string | 网关服务器地址            | `"271.22.65.1"`       |
    | `port`       | string    | 网关服务器端口            | `"443"`                      |
    | `error`      | int | 错误信息（成功时为空）    | `"1009"`    |

- 补充  

    无

`/api/v1/send_vrf`

- 请求参数（JSON）

    | 参数名       | 类型   | 必填 | 说明                | 示例值               |
    | :----------- | :----- | :--- | :-------------------| :------------------- |
    | `email`      | string | 是   | 需要验证的邮箱地址  | `"user@domain.com"`  |

- 返回值（JSON）

    | 参数名           | 类型   | 说明                      | 示例值                     |
    | :--------------- | :----- | :------------------------ | :------------------------- |
    | `target_email`   | string | 实际发送验证码的邮箱      | `"user@domain.com"`        |
    | `error`          | int | 错误信息（成功时为空）    | `1006`   |

- 补充  

    验证码有效期为3分钟

`/api/v1/register`

- 请求参数（JSON）

    | 参数名        | 类型   | 必填 | 说明                          | 示例值               |
    | :------------ | :----- | :--- | :-----------------| :------------------- |
    | `username`    | string | 是   | 用户名            | `"new_user"`         |
    | `email`       | string | 是   | 注册邮箱          | `"user@domain.com"`  |
    | `password`    | string | 是   | 前端加密后的密码   | `"6cgj__qtz3"`      |
    | `repassword`  | string | 是   | 重复加密后的密码   | `"6cgj__qtz3"`      |
    | `vrf_code`    | string | 是   | 4位数字验证码      | `"19nj"`           |

- 返回值（JSON）

    | 参数名           | 类型   | 说明                          | 示例值                     |
    | :--------------- | :----- | :---------------------------- | :------------------------- |
    | `target_uuid`    | int | 新创建用户的 uuid              | `2`              |
    | `target_email`   | string | 实际注册的邮箱地址            | `"user@domain.com"`        |
    | `error`          | int | 错误信息（成功时为空）        | `1003`          |

- 补充  

    `password` 和 `repassword` 字段必须在前端加密后传输，后端不存储明文密码

---

## ` ChatServer ` 的接口

**GET**

`/trans_ans` *WebSocket Upgrade*

- 请求参数  

    | 参数名        | 必填 | 说明                          | 示例值               |
    | :------------ | :--- | :-----------------------------| :------------------- |
    | `from`        |  是   | 标识升级请求方          | `api_server`         |
    | `session_id`       | 是   | 用于链接对应 Websocket 进行转发 | `1145`  |

- 返回值  

    Websocket 的自动升级响应

- 补充  

    需要传参  
    `from` 参数只能是 `api_server` 或者 `client`

## HTTP 请求反馈错误码一览

> 如果返回体是 JSON 格式，则会以 "error" 字段存储  

> 斜体的 *trs* 表示是转发其他服务器的错误码

| int32值 | 名称 | 描述 |
| :- | :- | :- |
| 0 | Success | 正常处理请求 |
| 1 | ErrorException | ` GateServer ` 中产生未定义错误 |
| 101 | ErrorRedis *trs* | ` VerifiServer ` 调用 Redis 出现错误 |
| 102 | ErrorSend *trs* | ` VerifiServer ` 未能成功发送验证邮件  |
| 103 | ErrorException *trs* | ` VerifiServer ` 未定义异常 |
| 1001 | ErrorServerNotResponding | ` GateServer ` 未收到其他服务器的响应 |
| 1002 | ErrorGrpc | ` GateServer ` 调用 gRPC 出现错误 |
| 1003 | ErrorJson | ` GateServer ` 处理前端传输  JSON 出现错误 |
| 1004 | ErrorMySql | ` GateServer ` 调用 MySQL 时发生异常 |
| 1005 | ErrorUsernameExists | ` GateServer ` 无法注册新用户：用户名已存在 |
| 1006 | ErrorEmailConflicts | ` GateServer ` 无法注册新用户：email已被注册 |
| 1007 | ErrorPwdIncorreponds | ` GateServer ` 无法注册新用户：密码不一致 |
| 1008 | ErrorVrfInvalid | ` GateServer ` 无法注册新用户：验证码无效 |
| 1009 | ErrorPwdWrong | ` GateServer ` 无法登录用户：密码错误 |
| 1010 | ErrorEmailInvalid | ` GateServer ` 无法登录用户：email 未注册 |
| 2001 | ErrorWebsocketUpgradeDinied | ` ChatServer ` 拒绝升级 WebSocket |

## 对 Python ` ApiServer ` 希望的接口

> 斜体是暂时不确定的部分

**GET**

` /get_response `

- 请求参数（JSON）

    | 参数名       | 类型   | 必填 | 说明                                                            | 示例值                    |
    | :----------- | :------ | :---- | :--------------------------------------------------------------- | :------------------------- |
    | `session_id` | int | 是 | 当前话语内容所属的对话 ID |  `1145` |
    | `model_class`   | string | 是   | 选择的模型 ID                                                   | `"claude-3-haiku"`        |
    | `model_id`   | string | 是   | 选择的模型大类                                                   | `"gemini"`        |
    | `prompt`     | array | 是   | 用户输入的提示内容                                              | `"{"role": "system","content": "你好，你想让我做什么？"}"`      |
    | `parameters` | object | 是   | 调用参数，如 temperature, thinking, online 等等                 | {"temperature": 0.7, ...} |
    | `url` | string | 否   | 自定义模型调用网址                 | `"https://api.deepseek.com"` |
    | `api_key` | string | 否   | 自定义模型调用api key                 | `"sk-176d442796bf4b4f9cf28afdb5r7438fhus"` |

- 返回值（text/plain; charset=utf-8）

    值只能为 "ready" 或者 "error"  
    指示是否已成功处理发送的对话信息，并且即将发起 Websocket 升级请求进行 AI 回答内容的传输

- 补充

    ` ChatServer ` 并不会特别准备 Websocket 的链接，需要 ` ApiServer ` 自行发起 Websocket 连接，然后发送内容到 ` ChatServer `
