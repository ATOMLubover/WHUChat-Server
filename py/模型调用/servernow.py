import asyncio
import json
import logging
import websockets
import aiohttp
import ssl
from aiohttp import web

import tiangong, gptreadimage, deepseekfunc, gptfunc
import tongyi, gemini, doubao, default, claude

HOST = "127.0.0.1"
PORT = 8000

logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")


async def fetch_message_history(uuid: int, session_id: int | None):
    url = "http://127.0.0.1:8000/api/v1/chat/browse_messages"
    payload = {"uuid": uuid, "session_id": session_id}
    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=payload) as resp:
            if resp.status == 200:
                return await resp.json()
            else:
                logging.warning(f"拉取历史消息失败: 状态码 {resp.status}")
                return {"error": 1, "messages": []}


async def handle_websocket(websocket):
    logging.info("WebSocket 连接已建立")
    try:
        async for message in websocket:
            logging.info(f"收到消息: {message}")
            data = json.loads(message)
            uuid = data.get("uuid")
            session_id = data.get("session_id")
            history = await fetch_message_history(0, session_id)
            messages = history.get("messages")
            promote = [{"role": msg["role"], "content": msg["content"]} for msg in messages]
            model_type = data.get("model")
            model_class = data.get("class")
            api_key = data.get("api_key")
            URL = data.get("URL")
            parameters = data.get("parameters", {})
            temperature = parameters.get("temperature", 0.7)
            talktype = parameters.get("type", "chat")

            match talktype:
                case "chat":
                    match model_class:
                        case "deepseek":
                            result = deepseekfunc.deepseekgate(model_type, promote, temperature)
                        case "chatgpt":
                            result = gptfunc.chatgpt_chat(model_type, promote, temperature)
                        case "tongyi":
                            result = tongyi.tongyi_gate(model_type, promote, temperature)
                        case "gemini":
                            result = gemini.generate_content_stream(model_type, promote, temperature)
                        case "doubao":
                            result = doubao.get_chat_completion(model_type, promote, temperature)
                        case "claude":
                            result = claude.stream_claude_response(promote, temperature)
                        case "tiangong":
                            result = tiangong.doubao_stream_chat(promote, temperature)
                        case _:
                            result = default.get_chat_completion(api_key, URL, model_type, promote, temperature)

                case "image":
                    match model_class:
                        case "chatgpt":
                            result = gptreadimage.chatgpt_chat(model_type, promote, temperature)
                        case "qianwen":
                            result = tongyi.tongyi_mutichat(promote, temperature)

                case "audio" | "video":
                    match model_class:
                        case "qianwen":
                            result = tongyi.tongyi_mutichat(promote, temperature)

            reasoning_chunks, content_chunks = [], []
            for chunk in result:
                if isinstance(chunk, dict):
                    if chunk.get("type") == "reasoning":
                        reasoning_chunks.append(chunk.get("reasoning_content", ""))
                    elif chunk.get("type") == "content":
                        content_chunks.append(chunk.get("content", ""))
                    else:
                        logging.warning(f"未知类型 chunk: {chunk}")
                else:
                    logging.warning(f"非字典 chunk: {chunk}")

            if reasoning_chunks:
                await websocket.send(json.dumps({"role": "reasoning", "reasoning_content": "".join(reasoning_chunks)}))
            if content_chunks:
                await websocket.send(json.dumps({"role": "content", "content": "".join(content_chunks)}))
            await websocket.send(json.dumps({"end": True}))
            await websocket.close()
            logging.info("WebSocket 连接已关闭")

    except websockets.exceptions.ConnectionClosed:
        logging.info("WebSocket 连接已关闭")
    except Exception as e:
        logging.error(f"WebSocket 发生错误: {e}")

async def handle_http_trigger(request):
    try:
        data = await request.json()
        target_wss = data.get("wss_url")  
        payload = data.get("payload", {})

        asyncio.create_task(trigger_wss_as_client(target_wss, payload))

        return web.json_response({"status": "ok", "message": f"已尝试连接 {target_wss}"})

    except Exception as e:
        logging.error(f"触发失败: {e}")
        return web.json_response({"status": "error", "message": str(e)}, status=500)

async def trigger_wss_as_client(wss_url: str, payload: dict):
    try:
        ssl_ctx = ssl.create_default_context() if wss_url.startswith("wss") else None
        async with websockets.connect(wss_url, ssl=ssl_ctx) as ws:
            await ws.send(json.dumps(payload))
            logging.info(f"作为客户端已向 {wss_url} 发送: {payload}")

            async for msg in ws:
                logging.info(f"收到响应: {msg}")
    except Exception as e:
        logging.error(f"WSS 客户端连接失败: {e}")

async def main():
    ws_server = websockets.serve(handle_websocket, HOST, PORT)
    app = web.Application()
    app.router.add_post("/trigger_ws", handle_http_trigger)

    await asyncio.gather(
        ws_server,
        web._run_app(app, port=8080)
    )

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("服务终止")
