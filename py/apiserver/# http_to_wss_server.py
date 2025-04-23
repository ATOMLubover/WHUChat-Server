# https_to_wss_server.py
import aiohttp
import asyncio
import ssl
import json
import logging
from aiohttp import web
import tiangong
import gptreadimage
import deepseekfunc
import gptfunc
import tongyi
import os
# import gemini
import doubao
import default
import claude


logging.basicConfig(level=logging.INFO)

# SSL 证书路径
#CERT_PATH = "py/apiserver/server.crt"
#KEY_PATH = "py/apiserver/server.key"

async def ensure_async_iterable(obj):
    if hasattr(obj, "__aiter__"):
        return obj  # 是 async generator
    async def fake_async_gen():
        for item in obj:
            yield item
    return fake_async_gen()
# 示例: 构造 WebSocket 连接地址（客户端暴露的 wss 服务地址）
def get_client_ws_url(request, session_id: int) -> str:
    client_ip = request.remote or "localhost"  # 取发起请求者的 IP
    return f"wss://{client_ip}:{wssport}/ws?session_id={session_id}"

# HTTPS 请求处理逻辑
async def handle_send_ans(request: web.Request):
    try:
        try:
            data = await request.json()
        except Exception as e:
            logging.error(f"请求体不是合法 JSON: {e}")
            return web.json_response(
                {"errorcode": 3001, "message": "请求体必须为合法 JSON"}
            )
        
        logging.info(f"接收到 HTTP 请求: {data}")
        session_id = data.get("session_id")
        if not session_id:
            return web.json_response({"error": 1, "message": "缺少 session_id"})


        required_fields = ["uuid", "session_id", "model_id", "model_class", "prompt"]
        missing_fields = [field for field in required_fields if field not in data]
        if missing_fields:
            message = f"缺少必要参数: {', '.join(missing_fields)}"
            logging.error(message)
            return web.json_response({"error": 3002, "message": message})
        # 构造 wss URL（你也可以替换成固定地址）
        ws_url = get_client_ws_url(request, session_id)
        logging.info(f"准备连接 WebSocket: {ws_url}")

        # 建立 wss 连接（服务端作为客户端连接目标 WebSocket 服务）
        client_ssl = ssl.create_default_context()
        client_ssl.check_hostname = False
        client_ssl.verify_mode = ssl.CERT_NONE

        async with aiohttp.ClientSession() as session:
            async with session.ws_connect(ws_url, ssl=client_ssl) as ws:
                await ws.send_str(f"来自服务端：已建立连接，session_id={session_id}")
                try:
                    uuid = data.get("uuid")
                    session_id = data.get("session_id")
                    model_id = data.get("model_id")
                    model_class = data.get("class")
                    api_key = data.get("api_key")
                    URL = data.get("URL")
                    parameters = data.get("parameters", {})
                    temperature = parameters.get("temperature", 0.7)
                    talktype = parameters.get("type", "chat")
                    #history = await fetch_message_history(0, session_id)
                    #messages = history.get("messages")
                    prompt_data = data.get("prompt", {})

                    # 如果是单条消息（dict），包装成列表
                    if isinstance(prompt_data, dict):
                        messages = [prompt_data]
                    elif isinstance(prompt_data, list):
                        messages = prompt_data
                    else:
                        messages = []

                    promote = [{"role": m["role"], "content": m["content"]} for m in messages]

                    if model_id == 1:
                        model_type = "deepseek-chat"

                    match talktype:
                        case "chat":
                            match model_class:
                                case "deepseek":
                                    print("deepseek")
                                    result = deepseekfunc.deepseekgate(model_type, promote, temperature)
                                case _:
                                    result = default.get_chat_completion(api_key, URL, model_type, promote, temperature)

                    has_sent_reasoning_header = False
                    has_sent_content_header = False
                    reasoning_buffer = []
                    content_buffer = []
                    result = await ensure_async_iterable(result)
                    
                    async for chunk in result if hasattr(result, "__aiter__") else result:
                        if isinstance(chunk, dict):
                            if chunk.get("type") == "reasoning":
                                if not has_sent_reasoning_header:
                                    await ws.send_str("Reasoning:")
                                    has_sent_reasoning_header = True
                                reasoning_text = chunk.get("reasoning_content", "")
                                if reasoning_text:
                                    await ws.send_str(reasoning_text)
                                    reasoning_buffer.append(reasoning_text)

                            elif chunk.get("type") == "content":
                                if not has_sent_content_header:
                                    await ws.send_str("Contents:")
                                    has_sent_content_header = True
                                content_text = chunk.get("content", "")
                                if content_text:
                                    await ws.send_str(content_text)
                                    content_buffer.append(content_text)
                            else:
                                logging.warning(f"未知类型 chunk: {chunk}")
                        else:
                            logging.warning(f"非字典 chunk: {chunk}")

                    if reasoning_buffer:
                        await ws.send_str("######@@@@@@%%%%%%")
                        logging.info("发送 reasoning 分隔符完成")

                    if content_buffer:
                        await ws.send_str("&&&&&&******^^^^^^")
                        logging.info("发送 content 分隔符完成")

                    await ws.send_str("*^%$%&&$$$$$$%^^$##E%##^^$#$%")
                    logging.info("发送 end 标志完成，关闭连接")

                except Exception as e:
                    logging.error(f"推送过程中出错: {e}")
                logging.info("消息已发送到 WebSocket 客户端")

        return web.json_response({"error": 0, "message": "WebSocket连接成功并发送消息"})

    except Exception as e:
        logging.error(f"处理失败: {e}")
        return web.json_response({"error": 500, "message": str(e)})

# 启动 HTTPS 服务监听 POST
def main():
    with open("py/apiserver/#http_to_wss_server.json", "r") as f:
        config = json.load(f)
    app = web.Application()
    app.router.add_post("/api/v1/ws/send_ans", handle_send_ans)
    global CERT_PATH, KEY_PATH,httpport, wssport
    CERT_PATH = config["CERT_PATH"]
    KEY_PATH = config["KEY_PATH"]
    httpport = config["httpport"]
    wssport = config["wssport"]
    #ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    #ssl_ctx.load_cert_chain(CERT_PATH, KEY_PATH)

    web.run_app(app, host="0.0.0.0", port=httpport)

if __name__ == "__main__":
    main()
