# https_to_wss_server.py
import aiohttp
import asyncio
import ssl
import json
import logging
from aiohttp import web
import kimi
import gptreadimage
import deepseekfunc
import gptfunc
import tongyi
import os
import configparser
import geminireasoner
import gemini
import doubao
import default
import claude
import sonar

logging.basicConfig(level=logging.INFO)

# SSL 证书路径
# CERT_PATH = "py/apiserver/server.crt"
# KEY_PATH = "py/apiserver/server.key"

async def convert_promote_to_message(promote):
    message = []
    for item in promote:
        role = item.get("role")
        contents = item.get("content", [])
        # 只提取所有 type == 'text' 的 text 字段并拼接
        text = "".join(c.get("text", "") for c in contents if c.get("type") == "text")
        message.append({"role": role, "content": text})
    return message
async def ensure_async_iterable(obj):
    if hasattr(obj, "__aiter__"):
        return obj  # 是 async generator

    async def fake_async_gen():
        for item in obj:
            yield item

    return fake_async_gen()


async def fetch_message_history(uuid: int, session_id: int | None):
    url = historyURL
    payload = {
        "uuid": uuid,
        "session_id": session_id,
    }
    client_ssl = ssl.create_default_context()
    client_ssl.check_hostname = False
    client_ssl.verify_mode = ssl.CERT_NONE
    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=payload, ssl=client_ssl) as resp:
            if resp.status == 200:
                return await resp.json()
            else:
                logging.warning(f"拉取历史消息失败: 状态码 {resp.status}")
                return {"error": 1, "messages": []}


# 示例: 构造 WebSocket 连接地址（客户端暴露的 wss 服务地址）
def get_client_ws_url(request, session_id: int) -> str:
    client_ip = request.remote or "localhost"  # 取发起请求者的 IP
    if client_ip == "::1":
        client_ip = "localhost"
    return f"wss://{client_ip}:{wssport}/api/v1/ws/send_ans?session_id={session_id}"


async def handle_wss_stream(data: dict, request: web.Request):
    try:
        session_id = data["session_id"]
        ws_url = get_client_ws_url(request, session_id)
        logging.info(f"准备连接 WebSocket: {ws_url}")

        client_ssl = ssl.create_default_context()
        client_ssl.check_hostname = False
        client_ssl.verify_mode = ssl.CERT_NONE

        async with aiohttp.ClientSession() as session:
            async with session.ws_connect(ws_url, ssl=client_ssl) as ws:
                # uuid = data.get("uuid")
                model_id = data.get("model_id")
                # model_class = data.get("model_class") or data.get("class")
                api_key = data.get("api_key")
                URL = data.get("URL")
                parameters = data.get("parameters", {})
                temperature = parameters.get("temperature", 0.7)
                # talktype = parameters.get("type", "chat")
                enableWebSearch = parameters.get("enableWebSearch", False)
                frugalMode = parameters.get("frugalMode", False)
                max_tokens = parameters.get("max_tokens", 1024)
                presence_penalty = parameters.get("presence_penalty", 0.0)
                top_p =  parameters.get("top_p", 1.0)

                # prompt_data = data.get("prompt", {})
                # prompt_data = await fetch_message_history(0, session_id)
                # print(f"获取的 prompt_data: {prompt_data}")  # 打印原始历史记录
                if frugalMode == False:
                    prompt_data1 = prompt_data.get("messages", [])
                    promotes = []
                    for msg in prompt_data1:
                        # Get the role from the message object
                        role = msg["sender"]

                        # Get the prompt data, which can be a list or a single item
                        prompt_data2 = msg["prompt"]

                        # Prepare the list for the 'content' field in the output
                        content_parts = []

                        # Handle both list and non-list prompt formats from the input
                        if isinstance(prompt_data2, list):
                            # If prompt_data is a list, iterate through its elements (prompt parts)
                            for part in prompt_data2:
                                if part["type"] == "text":
                                    # Transform text part format
                                    content_parts.append(
                                        {"type": "text", "text": part["text"]}
                                    )
                                elif part["type"] == "image":
                                    # Transform image part format to image_url with nested url
                                    # Assuming part["content"] already contains the base64 image data or URL
                                    content_parts.append(
                                        {
                                            "type": "image_url",
                                            "image_url": {"url": part["image_url"]},
                                        }
                                    )
                                # Add handling for other content types if necessary
                                # else:
                                #     # Optional: handle or skip unknown part types
                                #     pass
                        else:
                            # If prompt_data is not a list, assume it's a single text string
                            # Wrap it in the required list format for text type
                            content_parts.append({"type": "text", "text": prompt_data1})

                        # Append the complete message entry to the promotes list
                        # Only append if there's actual content
                        if content_parts:
                            promotes.append({"role": role, "content": content_parts})

                    print(f"构造出的 promote: {promotes}")  # 打印最终用于推理的消息内容
                else:
                    sender = data.get("sender")
                    prompt_data2 = data.get("prompt")
                    content_parts = []

                    # 遍历 prompt 中的每个部分进行格式转换
                    if isinstance(prompt_data2, list):  # 确保 prompt_data 确实是列表
                        for part in prompt_data2:
                            if part.get("type") == "text":
                                # 转换文本格式：content -> text
                                content_parts.append(
                                    {"type": "text", "text": part.get("text", "")}
                                )
                            elif part.get("type") == "image":
                                # 转换图片格式：image -> image_url, content -> image_url.url
                                # 假设 content 字段包含图片数据（base64 或 URL）
                                content_parts.append(
                                    {
                                        "type": "image_url",
                                        "image_url": {"url": part.get("text", "")},
                                    }
                                )
                            # 根据需要添加其他类型的处理
                            # else:
                            #     # 可选：处理未知类型或跳过
                            #     pass

                    # 构建包含当前消息的 promotes 列表
                    # promotes 将是一个包含一个元素的列表，这个元素代表当前消息
                    # 如果 content_parts 为空，则 promotes 列表也可能为空
                    promotes = (
                        [{"role": sender, "content": content_parts}]
                        if content_parts
                        else []
                    )
                promotes = await convert_promote_to_message(promotes)
                print(f"构造出的 promotes: {promotes}") 
                config = configparser.ConfigParser()
                config.read("py/apiserver/model_map.ini")
                model_id_map = dict(config["models"])
                model_id = str(data.get("model_id"))
                model_type = model_id_map.get(model_id)

                if not model_type:
                    return web.json_response({"error": 3006})

                match model_type:
                    case "deepseek-chat":
                        result = deepseekfunc.deepseek_chat(promotes, temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p)
                    case "gpt-3.5":
                        result = gptfunc.chatgpt_chat3(
                            temperature=temperature, enableWebSearch=enableWebSearch, messages=promotes,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "gpt-4.1":
                        result = gptfunc.chatgpt_chat4(
                            temperature=temperature, enableWebSearch=enableWebSearch, messages=promotes,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "o4-mini":
                        result = gptfunc.chatgpt_chatreasoning(
                            temperature=temperature, enableWebSearch=enableWebSearch, messages=promotes,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "claude-v1.3":
                        result = claude.stream_claude_response(messages=promotes,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p)
                    case "claude-3-7-sonnet-20250219":
                        result = claude.stream_claude_response(
                            messages=promotes, model="claude-3-7-sonnet-20250219",max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "deepseek-reasoner":
                        result = deepseekfunc.deepseek_chatreasoner(
                            promotes, temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "doubao-1-5-pro":
                        result = doubao.doubao_completion(
                            # model="doubao-1-5-pro",
                            messages=promotes,
                            temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "doubao-1-5-thinking-pro":
                        result = doubao.doubao_reasoner(
                            # model="doubao-1-5-thinking-pro",
                            messages=promotes,
                            temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "gemini-2.5-pro-exp-03-25":
                        result = gemini.gemini_chat(
                            messages=promotes, temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "gemini-2.5-flash-preview-04-17":
                        result = geminireasoner.gemini_chat(
                            messages=promotes, temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "kimi-latest":
                        result = kimi.kimi_chat(
                            messages=promotes,
                            temperature=temperature,
                            enableWebSearch=enableWebSearch,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "moonshot-v1-128k":
                        result = kimi.moonshot_chat(
                            messages=promotes,
                            temperature=temperature,
                            enableWebSearch=enableWebSearch,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "sonar":
                        result = sonar.sonar_chat(
                            messages=promotes, temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "sonarpro":
                        result = sonar.sonarpro(
                            messages=promotes, temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "qwen-max":
                        result = tongyi.tongyi_chat(
                            messages=promotes,
                            temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "qwq-plus":
                        result = tongyi.tongyi_reasoner(
                            messages=promotes,
                            temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case "kimi-reasoner":
                        result = kimi.kimi_reasoner(
                            messages=promotes,
                            temperature=temperature,
                            enableWebSearch=enableWebSearch,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )
                    case _:
                        result = default.get_chat_completion(
                            api_key=api_key, base_url=URL, model_type=model_type, promotes=promotes, temperature=temperature,max_tokens=max_tokens,presence_penalty=presence_penalty,top_p=top_p
                        )

                has_sent_reasoning_header = False
                has_sent_content_header = False
                reasoning_buffer = []
                content_buffer = []
                result = await ensure_async_iterable(result)
                await ws.send_str("\u001c\u001c\u001c")
                async for chunk in result:
                    if isinstance(chunk, dict):
                        if not has_sent_content_header:
                            await ws.send_str("\u001c\u001c\u001c")
                            has_sent_reasoning_header = True
                        if chunk.get("type") == "reasoning":
                            # await ws.send_str("\u200C\u001C\u200C")
                            if not has_sent_reasoning_header:
                                await ws.send_str("\u200c\u200c\u200c")
                                has_sent_reasoning_header = True
                            if reasoning_text := chunk.get("reasoning_content", ""):
                                await ws.send_str(reasoning_text)
                                reasoning_buffer.append(reasoning_text)

                        elif chunk.get("type") == "content":
                            # await ws.send_str("&^%$#@!()&")
                            if not has_sent_content_header:
                                # await ws.send_str("\u001c\u001c\u001c")
                                has_sent_content_header = True
                            if content_text := chunk.get("content", ""):
                                await ws.send_str(content_text)
                                content_buffer.append(content_text)
                    else:
                        logging.warning(f"非字典 chunk: {chunk}")

                if reasoning_buffer:
                    await ws.send_str("\u200c\u001c\u200c")
                    logging.info("发送 reasoning 分隔符完成")

                if content_buffer:
                    # await ws.send_str("\u001c\u200c\u001c")
                    logging.info("发送 content 分隔符完成")

                await ws.send_str("\u001c\u200c\u001c")
                logging.info("发送 end 标志完成")

    except Exception as e:
        logging.error(f"[推送线程异常] {e}")
        return web.json_response({"error": 3003})


# HTTPS 请求处理逻辑
async def handle_send_ans(request: web.Request):
    try:
        try:
            data = await request.json()
        except Exception as e:
            logging.error(f"请求体不是合法 JSON: {e}")
            return web.json_response({"error": 3001})

        logging.info(f"接收到 HTTP 请求: {data}")
        session_id = data.get("session_id")
        if not session_id:
            return web.json_response({"error": 3004})

        required_fields = ["uuid", "session_id", "model_id", "prompt"]
        missing_fields = [field for field in required_fields if field not in data]
        if missing_fields:
            return web.json_response({"error": 3002})

        global prompt_data
        prompt_data = await fetch_message_history(0, session_id)
        print(f"获取的 prompt_data: {prompt_data}")  # 打印原始历史记录
        error = prompt_data.get("error")
        if not prompt_data or error != 0:
            return web.json_response({"error": 3007})

        asyncio.create_task(handle_wss_stream(data, request))

        return web.json_response({"error": 0})

    except Exception as e:
        logging.error(f"处理失败: {e}")
        return web.json_response({"error": 3005})


# 启动 HTTPS 服务监听 POST
def main():
    with open("py/apiserver/http_to_wss_server.json", "r") as f:
        config = json.load(f)
    app = web.Application()
    app.router.add_post("/get_response", handle_send_ans)
    global CERT_PATH, KEY_PATH, httpport, wssport, historyURL, Host
    wssport = config["wssport"]
    Host = config["Host"]
    # historyURL = config["historyURL"]
    CERT_PATH = config["CERT_PATH"]
    KEY_PATH = config["KEY_PATH"]
    httpport = config["httpport"]
    historyURL = f"https://{Host}:{wssport}/api/v1/chat/browse_messages"
    ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_ctx.load_cert_chain(CERT_PATH, KEY_PATH)

    web.run_app(app, host="localhost", port=httpport, ssl_context=ssl_ctx)


if __name__ == "__main__":
    main()
