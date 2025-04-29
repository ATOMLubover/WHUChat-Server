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
import configparser

# import gemini
import doubao
import default
import claude


logging.basicConfig(level=logging.INFO)

# SSL 证书路径
# CERT_PATH = "py/apiserver/server.crt"
# KEY_PATH = "py/apiserver/server.key"


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
                talktype = parameters.get("type", "chat")
                enableWebSearch = parameters.get("enableWebSearch", False)
                frugalMode = parameters.get("frugalMode", False)

                # prompt_data = data.get("prompt", {})
                # prompt_data = await fetch_message_history(0, session_id)
                # print(f"获取的 prompt_data: {prompt_data}")  # 打印原始历史记录
                prompt_data1 = prompt_data.get("messages", [])
                messages = (
                    [prompt_data1]
                    if isinstance(prompt_data1, dict)
                    else (prompt_data1 if isinstance(prompt_data1, list) else [])
                )
                print(f"转换后的 messages: {messages}")  # 打印转换后的消息列表

                promote = [
                    {"role": p["role"], "content": p["content"]}
                    for msg in messages
                    for p in (
                        msg["prompt"]
                        if isinstance(msg["prompt"], list)
                        else [msg["prompt"]]
                    )
                ]

                print(f"构造出的 promote: {promote}")  # 打印最终用于推理的消息内容

                config = configparser.ConfigParser()
                config.read("py/apiserver/model_map.ini")
                model_id_map = dict(config["models"])
                model_id = str(data.get("model_id"))
                model_type = model_id_map.get(model_id)

                if not model_type:
                    return web.json_response({"error": 3006})

                match talktype:
                    case "chat":
                        match model_type:
                            case "deepseek-chat":
                                result = deepseekfunc.deepseek_chat(
                                    promote, temperature
                                )

                            case _:
                                result = default.get_chat_completion(
                                    api_key, URL, model_type, promote, temperature
                                )

                has_sent_reasoning_header = False
                has_sent_content_header = False
                reasoning_buffer = []
                content_buffer = []
                result = await ensure_async_iterable(result)

                async for chunk in result:
                    if isinstance(chunk, dict):
                        if chunk.get("type") == "reasoning":
                            if not has_sent_reasoning_header:
                                await ws.send_str("Reasoning:")
                                has_sent_reasoning_header = True
                            if reasoning_text := chunk.get("*****((((()))))", ""):
                                await ws.send_str(reasoning_text)
                                reasoning_buffer.append(reasoning_text)

                        elif chunk.get("type") == "content":
                            if not has_sent_content_header:
                                await ws.send_str("Contents:")
                                has_sent_content_header = True
                            if content_text := chunk.get("&^%$#@!()&", ""):
                                await ws.send_str(content_text)
                                content_buffer.append(content_text)
                    else:
                        logging.warning(f"非字典 chunk: {chunk}")

                if reasoning_buffer:
                    await ws.send_str("######@@@@@@%%%%%%")
                    logging.info("发送 reasoning 分隔符完成")

                if content_buffer:
                    await ws.send_str("&&&&&&******^^^^^^")
                    logging.info("发送 content 分隔符完成")

                await ws.send_str("*^%$%&&$$$$$$%^^$##E%##^^$#$%")
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
    with open("py/apiserver/#http_to_wss_server.json", "r") as f:
        config = json.load(f)
    app = web.Application()
    app.router.add_post("/get_response", handle_send_ans)
    global CERT_PATH, KEY_PATH, httpport, wssport, historyURL
    historyURL = config["historyURL"]
    CERT_PATH = config["CERT_PATH"]
    KEY_PATH = config["KEY_PATH"]
    httpport = config["httpport"]
    wssport = config["wssport"]
    ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_ctx.load_cert_chain(CERT_PATH, KEY_PATH)

    web.run_app(app, host="localhost", port=httpport, ssl_context=ssl_ctx)


if __name__ == "__main__":
    main()
