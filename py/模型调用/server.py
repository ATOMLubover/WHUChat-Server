import http
from http.server import SimpleHTTPRequestHandler
from http.server import CGIHTTPRequestHandler
from http.server import ThreadingHTTPServer
from functools import partial
import contextlib
import socket
import sys
import os
import deepseekfunc
import json
import gptfunc
import tongyi
import gemini
import doubao
import default
# 定义一个支持IPv4和IPv6双栈的服务器类
class DualStackServer(ThreadingHTTPServer):
    def server_bind(self):
        # 尝试设置IPv6套接字选项，以支持IPv4和IPv6双栈
        with contextlib.suppress(Exception):
            self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        return super().server_bind()

# 定义一个自定义的请求处理类
class MyHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        # 获取请求体的长度
        content_length = int(self.headers['Content-Length'])
        # 读取请求体数据
        post_data = self.rfile.read(content_length)
        
        try:
            # 解析 JSON 数据
            data = json.loads(post_data)
            model_type = data.get("model", "未提供")  # string 类型
            model_class = data.get("class", "未提供")  # string 类型
            promote_list = data.get("promote", [])  # list<string> 类型
            api_key = data.get("api_key", "未提供")  # string 类型
            base_url = data.get("base_url", "未提供")  # string 类型

            match(model_class):
                case  "deepseek":
                    result = deepseekfunc.deepseek_chat(model_type,promote_list)
                case  "chatgpt":
                    result = gptfunc.chatgpt_chat(model_type,promote_list)
                case "tongyi":
                    result = tongyi.tongyi_chat(model_type,promote_list)
                case "gemini":##
                    result = gemini.gemini_chat(model_type,promote_list)
                case "doubao":
                    result = doubao.get_chat_completion(model_type, promote_list)
                case _:
                    result = default.get_chat_completion(api_key,base_url,model_type, promote_list)


            # 设置响应头
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Transfer-Encoding', 'chunked')  # 启用流式传输
            self.end_headers()

            # 逐步写入流数据
            for chunk in result:
                if isinstance(chunk, dict):  # 可能返回字典
                    chunk_text = json.dumps(chunk)
                else:
                    chunk_text = str(chunk)

                chunk_bytes = chunk_text.encode('utf-8')
                self.wfile.write(f"{len(chunk_bytes):X}\r\n".encode('utf-8'))  # 发送块大小（十六进制）
                self.wfile.write(chunk_bytes)  # 发送块数据
                self.wfile.write(b"\r\n")  # 发送块结束标志
                self.wfile.flush()  # 确保数据实时发送

            # 发送流结束标记
            self.wfile.write(b"0\r\n\r\n")
            self.wfile.flush()
        except json.JSONDecodeError:
            self.send_response(400)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({"error": "请求体必须是 JSON 格式"}).encode('utf-8'))

def run(server_class=DualStackServer,
        handler_class=MyHandler,
        port=8000,
        bind='127.0.0.1',
        cgi=False,
        directory=os.getcwd()):

    if cgi:
        handler_class = partial(MyHandler, directory=directory)
    else:
        handler_class = partial(MyHandler, directory=directory)

    with server_class((bind, port), handler_class) as httpd:
        print(
            f"Serving HTTP on {bind} port {port} "
            f"(http://{bind}:{port}/) ..."
        )
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nKeyboard interrupt received, exiting.")
            sys.exit(0)

run(port=8000, bind='127.0.0.1')