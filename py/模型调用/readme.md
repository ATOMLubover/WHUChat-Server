# 对 Python ` ApiServer ` 希望的接口

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

    ` ChatServer ` 并不会特别准备 Websocket 的链接，需要 ` ApiServer ` 自行发起 Websocket 连接，然后发送内容到 ` ChatServe