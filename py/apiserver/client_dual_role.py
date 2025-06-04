# client_dual_role.py
import aiohttp
import asyncio
import ssl
import json
import logging
from aiohttp import web, WSMsgType

logging.basicConfig(level=logging.INFO)

CERT_PATH = "py/apiserver/server.crt"
KEY_PATH = "py/apiserver/server.key"

# 🔁 HTTPS 客户端：POST 请求服务端
async def send_post_to_server():
    url = "https://localhost:8090/get_response"
    data = {'model_id': 11, 
            'parameters': {'frugalMode': True, 'reasonable': False, 'temperature': 0.7,'max_tokens': 1024, 'top_p': 1.0,'presence_penalty': 0.0}, 
            'prompt': [{'text': '测试一下通信', 'type': 'text'}], 
            'sender': 'user',
            'session_id': 67, 
            'uuid': 1}

    ssl_ctx = ssl.create_default_context()
    ssl_ctx.check_hostname = False
    ssl_ctx.verify_mode = ssl.CERT_NONE

    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=data, ssl=ssl_ctx) as resp:
            print(f"[HTTPS] 状态码: {resp.status}")
            print(f"[HTTPS] 响应: {await resp.text()}")

# 🔁 WebSocket 服务端：监听 wss://localhost:8765/ws
async def websocket_handler(request):
    query_sid = request.query.get("session_id")
    

    ws = web.WebSocketResponse()
    await ws.prepare(request)
    logging.info(f"[WSS] WebSocket 连接成功 session_id={query_sid}")

    async for msg in ws:
        if msg.type == WSMsgType.TEXT:
            logging.info(f"[WSS] 收到消息: {msg.data}")
        elif msg.type == WSMsgType.ERROR:
            logging.error(f"[WSS] 错误: {ws.exception()}")

    logging.info("[WSS] 连接关闭")
    return ws

async def start_wss_server():
    app = web.Application()
    app.router.add_get("/api/v1/ws/send_ans", websocket_handler)

    ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_ctx.load_cert_chain(CERT_PATH, KEY_PATH)

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, "localhost", 8081, ssl_context=ssl_ctx)
    await site.start()

    logging.info("✅ WSS 监听启动 wss://localhost:8081/api/v1/ws/send_ans")
    return runner

# 主函数
async def main():
    await start_wss_server()
    await asyncio.sleep(1)  # 给 WSS 启动一点时间
    await send_post_to_server()
    await asyncio.Event().wait()  # 保持运行

if __name__ == "__main__":
    asyncio.run(main())
