
# WebSocket 服务接口文档

## 1. WebSocket 连接

### 连接地址：
`ws://127.0.0.1:8000`

### 协议：
- 使用 **WebSocket** 协议进行客户端与服务器之间的实时通信。
- 支持双向流式数据传输。

### 连接方式：
客户端通过 WebSocket 协议连接至该地址。成功连接后，客户端和服务器可以进行数据流式交互。

---

## 2. 请求格式

客户端向服务器发送 JSON 格式的请求消息，内容包括用户信息、模型类型、消息内容等。

### 请求消息格式：

```json
{
    "uuid": "<用户唯一标识>",
    "session_id": "<会话ID，可选>",
    "model": "<使用的模型名称>",
    "class": "<模型类别，如 deepseek, chatgpt 等>",
    "api_key": "<API Key（如有需要）>",
    "URL": "<API 服务地址（如有需要）>",
    "parameters": {
        "temperature": "<温度值，用于控制生成内容的随机性，默认为0.7>",
        "type": "<消息类型，chat（对话）、image（图片）、audio（音频）、video（视频）>"
    }
}
```

**字段说明**：
- `uuid`: 用户唯一标识符，用于区分不同的用户。
- `session_id`: 当前会话的标识符。可以为空。
- `model`: 请求使用的模型名称（如 `chatgpt`, `deepseek` 等）。
- `class`: 模型类别，决定使用哪个模型进行处理（如 `deepseek`, `chatgpt` 等）。
- `api_key`: 如果需要验证，请提供 API Key。
- `URL`: 自定义模型 API 地址（如果需要）。
- `parameters`: 控制请求的参数：
  - `temperature`: 控制模型输出的随机性，范围是 0 到 1，默认值为 0.7。
  - `type`: 消息类型，支持 `chat`（对话）、`image`（图片）、`audio`（音频）、`video`（视频）。

---

## 3. 响应格式

服务器返回的响应消息为流式数据，分为两部分：`reasoning_content`（推理阶段内容）和 `content`（最终生成的内容）。

### 消息格式 1：`reasoning_content`

推理阶段生成的内容，流式返回。

```json
{
    "role": "reasoning",
    "reasoning_content": "<推理过程中的内容>"
}
```

**字段说明**：
- `role`: 固定为 `reasoning`，标识这是推理阶段的内容。
- `reasoning_content`: 实时生成的推理内容。

### 消息格式 2：`content`

生成结果内容，流式返回。

```json
{
    "role": "content",
    "content": "<最终生成的内容>"
}
```

**字段说明**：
- `role`: 固定为 `content`，标识这是生成的最终内容。
- `content`: 最终的生成内容，可以是对话回复、图像、音频或视频等。

### 消息格式 3：结束标识

当所有内容都发送完毕时，服务器发送结束标识。

```json
{
    "end": true
}
```

---

## 4. 请求与响应流程

### 4.1 客户端发送请求

客户端通过 WebSocket 连接发送一个 JSON 请求，格式如下：

```json
{
    "uuid": "123456",
    "session_id": 789,
    "model": "deepseek-chat",
    "class": "deepseek",
    "api_key": "your_api_key",
    "URL": "http://api.deepseek.com",
    "parameters": {
        "temperature": 0.8,
        "type": "chat"
    }
}
```

### 4.2 服务器返回响应

服务器根据请求生成推理过程（`reasoning_content`）和最终生成的内容（`content`），然后按以下顺序流式返回：

1. **推理阶段内容（`reasoning_content`）**：

```json
{
    "role": "reasoning",
    "reasoning_content": "正在推理生成内容..."
}
```

2. **生成的最终内容（`content`）**：

```json
{
    "role": "content",
    "content": "这是最终生成的对话内容。"
}
```

3. **结束标识**：

```json
{
    "end": true
}
```

---

## 5. 错误处理

### 5.1 请求格式错误

如果客户端发送的请求格式不符合要求，服务器会返回如下错误消息：

```json
{
    "error": "Invalid request format"
}
```

### 5.2 模型处理失败

如果模型处理失败，服务器会返回错误消息：

```json
{
    "error": "Model processing failed",
    "message": "详细错误信息"
}
```

### 5.3 连接中断

如果 WebSocket 连接中断，服务器会发送以下日志消息：

```
WebSocket 连接已关闭
```

---

## 6. 注意事项

1. **流式响应**：服务器会将推理阶段（`reasoning_content`）和生成阶段（`content`）的内容按顺序流式返回。客户端应按顺序接收并显示这些内容。
2. **消息顺序**：`reasoning_content` 会在 `content` 之前发送。客户端应确保可以区分推理阶段和生成内容，并适时显示。
3. **支持多种请求类型**：支持不同的请求类型，如 `chat`（对话）、`image`（图片）、`audio`（音频）、`video`（视频）。根据不同的请求类型，服务器会执行相应的处理。
4. **模型类别**：根据 `class` 字段，服务器会选择相应的模型进行处理。可以使用不同的模型如 `deepseek`、`chatgpt`、`gemini` 等。

---

## 7. WebSocket 服务端点

- **WebSocket 连接地址**：`ws://127.0.0.1:8000`
- **连接方式**：客户端通过 WebSocket 协议连接至该地址，发送请求消息，服务器根据请求返回流式响应。

---
