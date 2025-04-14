import asyncio
import json
import logging
import websockets
import aiohttp

import gptreadimage
import deepseekfunc
import gptfunc
import tongyi
import gemini
import doubao
import default
import claude

HOST = "127.0.0.1"
PORT = 8000

logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")

# 拉取历史消息接口
async def fetch_message_history(uuid: int, session_id: int | None):
    url = "http://127.0.0.1:8000/api/v1/chat/browse_messages"
    payload = {
        "uuid": uuid,
        "session_id": session_id,
    }
    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=payload) as resp:
            if resp.status == 200:
                return await resp.json()
            else:
                logging.warning(f"拉取历史消息失败: 状态码 {resp.status}")
                return {"error": 1, "messages": []}

# WebSocket 处理逻辑
async def handle_websocket(websocket):
    logging.info("WebSocket 连接已建立")

    try:
        async for message in websocket:
            logging.info(f"收到消息: {message}")
            data = json.loads(message)
            uuid = data.get("uuid")
            session_id = data.get("session_id")
            # 获取历史消息
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
                            result = tongyi.tongyi_chat(model_type, promote, temperature)
                        case "gemini":
                            result = gemini.gemini_chat(model_type, promote, temperature)
                        case "doubao":
                            result = doubao.get_chat_completion(model_type, promote, temperature)
                        case "claude":
                            result = claude.stream_claude_response(promote, temperature)
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

            for chunk in result:
                chunk_text = json.dumps(chunk) if isinstance(chunk, dict) else str(chunk)
                await websocket.send(chunk_text)

            await websocket.send(json.dumps({"end": True}))
            logging.info("数据流发送完毕，发送结束标识")

            await websocket.close()
            logging.info("WebSocket 连接已关闭")

    except websockets.exceptions.ConnectionClosed:
        logging.info("WebSocket 连接已关闭")
    except Exception as e:
        logging.error(f"WebSocket 发生错误: {e}")

# 启动 WebSocket 服务
async def run_websocket_server():
    async with websockets.serve(handle_websocket, HOST, PORT):
        logging.info(f"WebSocket 服务器已启动：ws://{HOST}:{PORT}")
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(run_websocket_server())
    except KeyboardInterrupt:
        logging.info("服务器已关闭")
