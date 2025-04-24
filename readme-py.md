
# HTTPS to WebSocket Server 接口文档

## 接口概述

该服务用于接收来自客户端的 HTTPS POST 请求，解析后异步连接客户端提供的 WebSocket 服务（wss://{client_ip}:{wssport}/api/v1/ws/send_ans），并将 AI 推理结果按类型（reasoning 和 content）进行流式发送。

---

## 1. 接口地址

### URL

```
POST /get_response
```

### 请求协议

HTTPS

---

## 2. 请求参数

请求体为 JSON 格式，包含以下字段：

| 字段名        | 类型      | 是否必填 | 描述 |
|---------------|-----------|----------|------|
| uuid          | int       | 是       | 用户或请求唯一标识符 |
| session_id    | int       | 是       | 会话标识，用于匹配 WebSocket 客户端地址 |
| model_id      | int       | 是       | 模型编号，例如 1 表示 deepseek-chat |
| model_class   | string    | 是       | 模型分类（如 deepseek、tongyi 等） |
| prompt        | dict/list | 是       | 对话上下文，支持单条或多条消息 |
| parameters    | dict      | 否       | 额外参数，例如 temperature 和 type |
| api_key       | string    | 否       | 指定模型 API 所需的密钥 |
| URL           | string    | 否       | 指定模型 API 的请求地址 |

示例请求体：

```json
{
  "uuid": 123456,
  "session_id": 7890,
  "model_id": 1,
  "model_class": "deepseek",
  "prompt": [
    {"role": "user", "content": "介绍一下量子计算"},
    {"role": "assistant", "content": "量子计算是一种..."}
  ],
  "parameters": {
    "temperature": 0.7,
    "type": "chat"
  }
}
```

---

## 3. 返回参数

### 成功返回（HTTP 200）

```json
{
  "error": 0
}
```

表示请求成功，AI 推理结果将通过 WebSocket 流式推送给客户端。

### 失败返回

| 错误码 | 描述                     |
|--------|--------------------------|
| 3001   | 请求体不是合法 JSON      |
| 3002   | 缺少必填字段             |
| 3003   | 推送线程异常             |
| 3004   | 未提供 session_id        |
| 3005   | 内部处理失败             |

---

## 4. WebSocket 消息格式

服务端会自动建立到客户端的 WebSocket 连接，并以如下格式发送消息：

### Reasoning 内容段

```text
Reasoning:
[reasoning 内容]
...
######@@@@@@%%%%%%
```

### Content 内容段

```text
Contents:
[内容文本]
...
&&&&&&******^^^^^^
```

### 最终结束标志

```text
*^%$%&&$$$$$$%^^$##E%##^^$#$%
```

---

## 5. 配置文件路径

配置文件路径为：

```
py/apiserver/#http_to_wss_server.json
```

### 示例配置内容：

```json
{
  "historyURL": "https://example.com/history",
  "CERT_PATH": "py/apiserver/server.crt",
  "KEY_PATH": "py/apiserver/server.key",
  "httpport": 8443,
  "wssport": 443
}
```

---

## 6. 启动说明

服务启动后将监听本地 `https://localhost:{httpport}/get_response` 端口，支持 POST 请求。

---
