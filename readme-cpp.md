# C++ 部分后端共享信息 README

## 后端大致架构

与前端直接交互的是 C++ boost::asio 和 boost::beast 开发的 ` GateServer ` 和 ` ChatServer `  
` GateServer ` 处理前端 Web 发送的 **HTTP 请求**，并决定如何调用其他服务器，获得其他服务器的处理结果，并最后综合返回 **HTTP 响应** 给 Web  
` ChatServer ` 处理前端 Web 发送的 **HTTP 请求**，并且决定是返回 **HTTP 响应** 给前端，还是升级为 **Websocket 连接**  

后端服务器调用关系：

` GateServer ` 通过 gRPC 调用 Node.js 开发的 ` VerifiServer ` 进行注册账号时的验证码发送  
` GateServer ` 通过 gRPC 调用 C++ 开发的 ` StatusServer ` 进行客户端分配，
随后指示客户端转调用 ` ChatServer ` 的接口进行数据处理  
` ChatServer ` 会通过 Websocoket 和 HTTP 与 ` ApiServer ` 交互进行 AI 接口调用处理

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

### GET

#### `/home`

- 请求参数  

    无

- 返回值

    无

- 补充  

    当浏览器携带正确的 cookie 时，会直接重定向到 /chat 页面  
    如果处理中发现有关 cookie 的问题，会直接重定向到 /login 页面

#### `/favicon.ico`

- 请求参数  

    无

- 返回值（application/octet-stream）

    返回网站图标二进制文件

- 补充  

    无

#### `/login`

- 请求参数  

    无

- 返回值（text/html）  

    返回登录页面 html 页面

- 补充  

    浏览器会自动获取其相关的 js 和 css 文件

#### `/register`

- 请求参数  

    无

- 返回值（text/html）  

    返回注册页面 html 页面

- 补充  

    浏览器会自动获取其相关的 js 和 css 文件

### POST

#### `/api/v1/login`

- 请求参数（JSON）

    | 参数名     | 类型   | 必填 | 说明         | 示例值              |
    | :--------- | :----- | :--- | :----------- | :------------------ |
    | `email`    | string | 是   | 用户注册邮箱 | `"user@domain.com"` |
    | `password` | string | 是   | 用户密码     | `"P@ssw0rd"`        |

- 返回值（none 或 JSON）  

    当登录不成功：  

    | 参数名  | 类型 | 说明         | 示例值 |
    | :------ | :--- | :----------- | :----- |
    | `uuid`  | int  | 用户唯一标识 | `2233` |
    | `error` | int  | 错误信息     | `1009` |

- 补充  

    当登录成功会直接重定向到 /chat 页面，且会返回用于免密登录的 cookie（含有 uuid，updated_at 和 token）  
    如果登录不成功才会返回 json 响应体  

#### `/api/v1/get_chatserver`

- 请求参数

    无

- 返回值（JSON）  

    | 参数名  | 类型   | 说明              | 示例值               |
    | :------ | :----- | :---------------- | :------------------- |
    | `addr`  | string | `ChatServer` 地址 | `"271.22.65.1:8081"` |
    | `error` | int    | 错误信息          | `1013`               |

- 补充  

    在发送该请求之前，应该先确定获得了有效的 cookie（在 /api/v1/login 成功之后会更新 cookie）  
    当由前端自行在合适的时间请求，从而获得能够连接 `ChatServer` 接口 （比如在 /chat 页面加载完成之后）  

#### `/api/v1/send_vrf`

- 请求参数（JSON）

    | 参数名  | 类型   | 必填 | 说明               | 示例值              |
    | :------ | :----- | :--- | :----------------- | :------------------ |
    | `email` | string | 是   | 需要验证的邮箱地址 | `"user@domain.com"` |

- 返回值（JSON）

    | 参数名         | 类型   | 说明                   | 示例值              |
    | :------------- | :----- | :--------------------- | :------------------ |
    | `target_email` | string | 实际发送验证码的邮箱   | `"user@domain.com"` |
    | `error`        | int    | 错误信息（成功时为空） | `1006`              |

- 补充  

    验证码有效期为3分钟

#### `/api/v1/register`

- 请求参数（JSON）

    | 参数名       | 类型   | 必填 | 说明             | 示例值              |
    | :----------- | :----- | :--- | :--------------- | :------------------ |
    | `username`   | string | 是   | 用户名           | `"new_user"`        |
    | `email`      | string | 是   | 注册邮箱         | `"user@domain.com"` |
    | `password`   | string | 是   | 前端加密后的密码 | `"6cgj__qtz3"`      |
    | `repassword` | string | 是   | 重复加密后的密码 | `"6cgj__qtz3"`      |
    | `vrf_code`   | string | 是   | 4位数字验证码    | `"19nj"`            |

- 返回值（JSON）

    | 参数名         | 类型   | 说明                   | 示例值              |
    | :------------- | :----- | :--------------------- | :------------------ |
    | `target_uuid`  | int    | 新创建用户的 uuid      | `2`                 |
    | `target_email` | string | 实际注册的邮箱地址     | `"user@domain.com"` |
    | `error`        | int    | 错误信息（成功时为空） | `1003`              |

- 补充  

    `password` 和 `repassword` 字段必须在前端加密后传输，后端不存储明文密码

---

## ` ChatServer ` 的接口（全为 HTTPS 或 WSS）

### GET

#### `/api/v1/ws/trans_ans` *WebSocket Upgrade*

- 请求参数  

    | 参数名       | 必填 | 说明                            | 示例值 |
    | :----------- | :--- | :------------------------------ | :----- |
    | `uuid`       | 是   | 当前用户                        | `123`  |
    | `session_id` | 是   | 用于链接对应 Websocket 进行转发 | `1145` |
    | `model_id`   | 是   | 当前获取答案的模型              | `2`    |

- 返回值  

    Websocket 的自动升级响应  
    当出现错误时时，返回 400 Bad Request

- 补充  

    Web 客户端使用的接口，会验证 cookie

#### `/api/v1/ws/send_ans` *WebSocket Upgrade*

- 请求参数  

    | 参数名       | 必填 | 说明                                       | 示例值 |
    | :----------- | :--- | :----------------------------------------- | :----- |
    | `session_id` | 是   | 用于链接对应 Websocket 进行转发            | `1145` |
    | `model_id`   | 是   | 数据库记录 AI 回答的标记（其实不重要）     | `1`    |

- 返回值  

    Websocket 的自动升级响应  
    当出现错误时，返回 400 Bad Request

- 补充  

    `ApiServer` 使用的接口，Web 客户端不能使用

#### `/api/v1/chat/models`

- 请求参数  

    无

- 返回值  

    | 参数名   | 类型     | 说明                       | 示例值 |
    | :------- | :------- | :------------------------- | :----- |
    | `models` | 对象数组 | 包含所有可用模型信息的数组 | 见下   |

    `models` 中对象的说明

    | 参数名  | 类型   | 说明         | 示例值             |
    | :------ | :----- | :----------- | :----------------- |
    | `id`    | int    | 模型序号     | `3`                |
    | `name`  | string | 模型具体名字 | `"DeepSeek V3"`    |
    | `class` | string | 模型的类别   | `"claude-3-haiku"` |
    | `desc`  | string | 模型描述     | `"A powerful LLM"` |

- 补充  

    暂时没有实现根据客户的等级返回模型的功能  
    另外由于使用了 cookie 进行验证，所以无须传递任何参数

### POST

#### `/api/v1/chat/send_message`

- 请求参数（application/json）

    | 参数名        | 类型   | 必填 | 说明                                                            | 示例值                                                     |
    | :------------ | :----- | :--- | :-------------------------------------------------------------- | :--------------------------------------------------------- |
    | `uuid`        | int    | 是   | 用户唯一标识                                                    | `1`                                                        |
    | `session_id`  | int    | 是   | 会话 ID（新对话时传递 null，由后端赋予，继续对话时传递已有 ID） | `1`                                                        |
    | `model_class` | string | 是   | 选择的模型 ID                                                   | `"claude-3-haiku"`                                         |
    | `model_id`    | string | 是   | 选择的模型大类                                                  | `"gemini"`                                                 |
    | `prompt`      | array  | 是   | 用户输入的提示内容                                              | `"{"role": "system","content": "你好，你想让我做什么？"}"` |
    | `parameters`  | object | 是   | 调用参数，如 temperature, thinking, online 等等                 | {"temperature": 0.7, ...}                                  |
    | `URL`         | string | 否   | 自定义模型调用网址                                              | `"https://api.deepseek.com"`                               |
    | `api_key`     | string | 否   | 自定义模型调用api key                                           | `"sk-176d442796bf4b4f9cf28afdb5r7438fhus"`                 |

- 返回值（application/json）

    | 参数名  | 类型 | 说明   | 示例值 |
    | ------- | ---- | ------ | ------ |
    | `error` | int  | 错误码 | `2002` |

- 补充

    Web 前端在接受到正常的 HTTP 响应后，要自行建立 Websocket 连接接受 ApiServer 的回答  

#### `/api/v1/chat/browse_messages`

- 请求参数（application/json）

    | 参数名       | 类型 | 必填 | 说明                                                            | 示例值 |
    | :----------- | :--- | :--- | :-------------------------------------------------------------- | :----- |
    | `uuid`       | int  | 是   | 用户唯一标识                                                    | `1`    |
    | `session_id` | int  | 是   | 会话 ID（新对话时传递 null，由后端赋予，继续对话时传递已有 ID） | `2`    |

- 返回值（application/json）

    | 参数名     | 类型     | 说明                              | 示例值 |
    | ---------- | -------- | --------------------------------- | ------ |
    | `error`    | int      | 错误码                            | `2002` |
    | `messages` | 对象数组 | 记录指定 session 中所有的 message | 见下   |

    `messages` 中对象的说明

    | 参数名        | 类型   | 说明                                                            | 示例值                                                     |
    | :------------ | :----- | :-------------------------------------------------------------- | :--------------------------------------------------------- |
    | `uuid`        | int    | 用户唯一标识                                                    | `1`                                                      |
    | `session_id`  | int    | 会话 ID（新对话时传递 null，由后端赋予，继续对话时传递已有 ID） | `2`                                                        |
    | `model_id` | int | 选择的模型 ID                                                  | `3`                                         |
    | `model_class`    | string | 选择的模型大类                                                  | `"OpenAI"`                                                 |
    | `prompt`      | array  | 用户输入的提示内容                                              | `"{"role": "system","content": "你好，你想让我做什么？"}"` |
    | `parameters`  | object | 调用参数，如 temperature, thinking, online 等等                 | {"temperature": 0.7, ...}                                  |
    | `URL`         | string | 自定义模型调用网址                                              | `"https://api.deepseek.com"`                               |
    | `api_key`     | string | 自定义模型调用api key                                           | `"sk-176d442796bf4b4f9cf28afdb5r7438fhus"`                 |

- 补充

    Web 前端和 ` ApiServer ` 均可以使用，用于获取特定会话的历史记录（注：` ApiServer ` 使用时务必设置 `uuid` 为 0）

#### `/api/v1/chat/history`

- 请求参数（application/json）

    | 参数名 | 类型 | 必填 | 说明         | 示例值 |
    | :----- | :--- | :--- | :----------- | :----- |
    | `uuid` | int  | 是   | 用户唯一标识 | `1`    |

- 返回值（application/json）

    | 参数名     | 类型     | 说明                       | 示例值 |
    | ---------- | -------- | -------------------------- | ------ |
    | `error`    | int      | 错误码                     | `2002` |
    | `sessions` | 对象数组 | 当前用户所有会话的基本信息 | 见下   |

    `sessions` 中对象的说明

    | 参数名       | 类型   | 说明               | 示例值             |
    | :----------- | :----- | :----------------- | :----------------- |
    | `uuid`       | int    | 用户唯一标识       | `1`                |
    | `id`         | int    | 会话 ID            | `123`              |
    | `title`      | string | 会话的标题         | `"claude-3-haiku"` |
    | `updated_at` | string | 最后一次更新的时间 | `"gemini"`         |

- 补充

    Web 前端用于获取用来简单显示的会话列表信息

## HTTP 请求反馈错误码一览

> 如果返回体是 JSON 格式，则会以 "error" 字段存储  
> 斜体的 *trs* 表示是转发其他服务器的错误码

| int32值 | 名称                        | 描述                                         |
| :------ | :-------------------------- | :------------------------------------------- |
| 0       | Success                     | 正常处理请求                                 |
| 1       | ErrorException              | ` GateServer ` 中产生未定义错误              |
| 101     | ErrorRedis *trs*            | ` VerifiServer ` 调用 Redis 出现错误         |
| 102     | ErrorSend *trs*             | ` VerifiServer ` 未能成功发送验证邮件        |
| 103     | ErrorException *trs*        | ` VerifiServer ` 未定义异常                  |
| 1001    | ErrorServerNotResponding    | ` GateServer ` 未收到其他服务器的响应        |
| 1002    | ErrorGrpc                   | ` GateServer ` 调用 gRPC 出现错误            |
| 1003    | ErrorJson                   | ` GateServer ` 处理前端传输  JSON 出现错误   |
| 1004    | ErrorMySql                  | ` GateServer ` 调用 MySQL 时发生异常         |
| 1005    | ErrorUsernameExists         | ` GateServer ` 无法注册新用户：用户名已存在  |
| 1006    | ErrorEmailConflicts         | ` GateServer ` 无法注册新用户：email已被注册 |
| 1007    | ErrorPwdIncorreponds        | ` GateServer ` 无法注册新用户：密码不一致    |
| 1008    | ErrorVrfInvalid             | ` GateServer ` 无法注册新用户：验证码无效    |
| 1009    | ErrorPwdWrong               | ` GateServer ` 无法登录用户：密码错误        |
| 1010    | ErrorEmailInvalid           | ` GateServer ` 无法登录用户：email 未注册    |
| 1011    | ErrorLoginCookieInvalid     | ` GateServer ` 拒绝访问：cookie 无效         |
| 1012    | ErrorCookieNotFound         | ` GateServer ` 未找到 cookie                 |
| 1013    | ErrorUnableGetServer        | ` GateServer `无法获取 ChatServer 地址       |
| 2001    | ErrorWebsocketUpgradeDinied | ` ChatServer ` 拒绝升级 WebSocket            |
| 2002    | ErrorSendCookieInvalid      | ` ChatServer ` 无法解析 cookie               |
| 2003    | ErrorApiNotResponding       | ` ChatServer ` 未接受到 ` ApiServer ` 的响应 |
| 2003    | ErrorSsnIdInvalid           | ` ChatServer ` 无法找到对应 session_id       |

## 对 Python ` ApiServer ` 希望的接口

> 斜体是暂时不确定的部分

### POST

` /get_response `

- 请求参数（JSON）

    | 参数名        | 类型   | 必填 | 说明                                                            | 示例值                                                     |
    | :------------ | :----- | :--- | :-------------------------------------------------------------- | :--------------------------------------------------------- |
    | `uuid`        | int    | 是   | 用户唯一标识                                                    | `1`                                                        |
    | `session_id`  | int    | 是   | 会话 ID（新对话时传递 null，由后端赋予，继续对话时传递已有 ID） | `2`                                                        |
    | `model_class` | string | 是   | 选择的模型 ID                                                   | `"claude-3-haiku"`                                         |
    | `model_id`    | string | 是   | 选择的模型大类                                                  | `"gemini"`                                                 |
    | `prompt`      | array  | 是   | 用户输入的提示内容                                              | `"{"role": "system","content": "你好，你想让我做什么？"}"` |
    | `parameters`  | object | 是   | 调用参数，如 temperature, thinking, online 等等                 | {"temperature": 0.7, ...}                                  |
    | `URL`         | string | 否   | 自定义模型调用网址                                              | `"https://api.deepseek.com"`                               |
    | `api_key`     | string | 否   | 自定义模型调用api key                                           | `"sk-176d442796bf4b4f9cf28afdb5r7438fhus"`                 |

- 返回值（application/json）

    | 参数名  | 类型 | 必填 | 说明                     | 示例值 |
    | :------ | :--- | :--- | :----------------------- | :----- |
    | `error` | int  | 是   | ApiServer 中产生的错误码 | `1`    |

- 补充

    参数是 Web 所发给 ChatServer 的请求，除了 session_id 进行了更新没有改变  
    返回值中的 ` error ` 的取值，请参照上一部分中的 POST 请求错误码表的形式，从 201 开始编号
    ` ChatServer ` 并不会特别准备 Websocket 的链接，需要 ` ApiServer ` 自行发起 Websocket 连接，然后发送内容到 ` ChatServer `
