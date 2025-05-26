# 接口文档：HTTP 到 WebSocket 推理转发服务

## 1. 接口概览

- **接口名称**：发送推理请求（转发至 WebSocket）
- **请求地址**：`POST /api/v1/send_ans`
- **请求协议**：HTTPS
- **内容类型**：`application/json`
- **用途**：接收 HTTP 推理请求，解析参数后作为 WebSocket 客户端连接指定地址，并将推理结果流式发送至客户端 WebSocket 连接。

---

## 2. 请求参数

### Header

| 参数名 | 类型 | 是否必须 | 描述 |
|--------|------|----------|------|
| Content-Type | string | 是 | 必须为 `application/json` |

### Body 参数（JSON 格式）

| 参数名 | 类型 | 是否必须 | 描述 |
|--------|------|----------|------|
| uuid | int | 是 | 用户唯一标识 |
| session_id | int | 是 | 会话唯一标识，用于匹配历史对话 |
| model_id | string | 是 | 模型标识（将通过 `model_map.ini` 映射为模型类型） |
| prompt | array | 是 | 当前对话的 prompt 内容，格式如下（可包含 text 或 image） |
| api_key | string | 否 | 若使用外部 API 模型需提供密钥 |
| URL | string | 否 | 若使用外部模型请求 URL |
| parameters | object | 否 | 额外参数对象，如温度等 |
| parameters.temperature | float | 否 | 模型温度，默认 0.7 |
| parameters.enableWebSearch | bool | 否 | 是否启用联网搜索，默认 false |
| parameters.frugalMode | bool | 否 | 是否只推送当前消息（跳过历史），默认 false |
| sender | string | frugalMode 为 true 时必需 | 当前发消息者角色（如 "user" 或 "assistant"） |

### prompt 内容格式

```json
[
  { "type": "text", "text": "Hello world" },
  { "type": "image", "text": "https://example.com/image.png" }
]
```

---

## 3. 响应格式（HTTP）

### 成功返回

```json
{ "error": 0 }
```

### 错误返回示例

| 错误码 | 含义 |
|--------|------|
| 3001 | 请求体不是合法 JSON |
| 3002 | 缺少必需字段 |
| 3003 | WebSocket 连接或模型推理异常 |
| 3004 | session_id 缺失 |
| 3006 | 无效的 model_id 映射 |

---

## 4. WebSocket 推送格式（客户端接收）

在建立 wss 连接后，服务器会按以下流式结构发送数据：

### 特殊标记

| 含义 | 标记内容 |
|------|-----------|
| 推理开始 | `` |
| reasoning 开始 | `‌‌‌` |
| reasoning 结束 | `‌‌` |
| content 结束 + 总体结束 | `‌` |

### 发送数据格式

```

‌‌‌
[推理内容...]
‌‌
[主内容...]
‌
```

---

## 5. 示例请求

```http
POST /api/v1/send_ans HTTP/1.1
Host: example.com
Content-Type: application/json

{
  "uuid": 1234,
  "session_id": 5678,
  "model_id": "gpt-4",
  "prompt": [
    { "type": "text", "text": "Who are you?" }
  ],
  "parameters": {
    "temperature": 0.7,
    "enableWebSearch": false,
    "frugalMode": false
  }
}
```