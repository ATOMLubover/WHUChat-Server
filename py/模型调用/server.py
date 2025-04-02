import http
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import contextlib
import socket
import sys
import os
import json
import asyncio
import websockets

import deepseekfunc
import gptfunc
import tongyi
import gemini
import doubao
import default

class DualStackServer(ThreadingHTTPServer):
    def server_bind(self):
        with contextlib.suppress(Exception):
            self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        return super().server_bind()

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.headers.get("Upgrade", "").lower() == "websocket":
            asyncio.create_task(handle_websocket(self))
        else:
            self.send_response(426) 
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "请使用 WebSocket 连接"}).encode("utf-8"))

async def handle_websocket(request_handler):
    client_socket = request_handler.request
    ws = websockets.server.WebSocketServerProtocol(client_socket)
    await ws.handshake(request_handler.headers, request_handler.path)

    try:
        async for message in ws:
            data = json.loads(message)
            model_type = data.get("model", "未提供")
            model_class = data.get("class", "未提供")
            promote_list = data.get("promote", [])
            api_key = data.get("api_key", "未提供")
            base_url = data.get("base_url", "未提供")

            match model_class:
                case "deepseek":
                    result = deepseekfunc.deepseek_chat(model_type, promote_list)
                case "chatgpt":
                    result = gptfunc.chatgpt_chat(model_type, promote_list)
                case "tongyi":
                    result = tongyi.tongyi_chat(model_type, promote_list)
                case "gemini":
                    result = gemini.gemini_chat(model_type, promote_list)
                case "doubao":
                    result = doubao.get_chat_completion(model_type, promote_list)
                case _:
                    result = default.get_chat_completion(api_key, base_url, model_type, promote_list)

            for chunk in result:
                if isinstance(chunk, dict):
                    chunk_text = json.dumps(chunk)
                else:
                    chunk_text = str(chunk)
                await ws.send(chunk_text)

    except Exception as e:
        print(f"WebSocket 错误: {e}")
    finally:
        await ws.close()

def run(port=8000, bind='127.0.0.1'):
    server_address = (bind, port)
    httpd = DualStackServer(server_address, MyHandler)
    
    print(f"HTTP/WebSocket 服务器运行中: http://{bind}:{port}/")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n服务器已停止。")
        sys.exit(0)

#run(port=8000, bind='127.0.0.1')
