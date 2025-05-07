import http.server
import socketserver
import json
from urllib.parse import urlparse

# --- 错误码定义 ---
ERR_SUCCESS = 0
ERR_NOT_FOUND = 1
ERR_METHOD_NOT_ALLOWED = 2
ERR_LENGTH_REQUIRED = 3
ERR_INVALID_CONTENT_LENGTH = 4
ERR_INVALID_JSON = 5
ERR_READ_BODY_FAILED = 6
ERR_PROCESSING_FAILED = 7

# --- 错误码到 HTTP 状态码和消息的映射 ---
ERROR_DETAILS = {
    ERR_SUCCESS: (200, "Success"),
    ERR_NOT_FOUND: (404, "Not Found"),
    ERR_METHOD_NOT_ALLOWED: (405, "Method Not Allowed"),
    ERR_LENGTH_REQUIRED: (411, "Length Required"),
    ERR_INVALID_CONTENT_LENGTH: (400, "Bad Request: Invalid Content-Length"),
    ERR_INVALID_JSON: (400, "Bad Request: Invalid JSON in body"),
    ERR_READ_BODY_FAILED: (500, "Internal Server Error: Failed to read body"),
    ERR_PROCESSING_FAILED: (500, "Internal Server Error: Processing failed"),
}

# 定义服务器监听的地址和端口
HOST = "localhost"
PORT = 8090


class SimpleHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    """
    处理 HTTP 请求，始终返回 JSON 响应，包含 error 码。
    """

    def _send_json_response(self, error_code, extra_data=None):
        """
        辅助方法，用于发送标准化的 JSON 响应。

        Args:
            error_code (int): 来自上面定义的错误码。
            extra_data (dict, optional): 可以在成功时附加到响应中的额外数据。
        """
        status_code, default_message = ERROR_DETAILS.get(
            error_code, (500, "Unknown Internal Server Error")
        )

        response_body = {"error": error_code, "message": default_message}
        if extra_data and error_code == ERR_SUCCESS:
            # 可以选择在成功时合并额外数据，但当前需求只需要 error: 0
            # response_body.update(extra_data)
            pass  # 保持响应体为 {"error": 0, "message": "Success"}

        try:
            self.send_response(status_code)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            response_bytes = json.dumps(response_body, ensure_ascii=False).encode(
                "utf-8"
            )
            self.wfile.write(response_bytes)
        except Exception as e:
            # 如果发送响应本身就失败了，只能在服务器端记录日志
            print(f"CRITICAL: Failed to send response (code {error_code}): {e}")

    def do_POST(self):
        """
        处理 POST 请求，解析 JSON 体，始终返回 JSON 响应。
        """
        parsed_path = urlparse(self.path)
        path = parsed_path.path

        if path == "/get_response":
            print(f"收到 POST 请求: {self.path}")

            # 1. 检查 Content-Length
            content_length_header = self.headers.get("Content-Length")
            if not content_length_header:
                self._send_json_response(ERR_LENGTH_REQUIRED)
                return
            try:
                content_length = int(content_length_header)
            except ValueError:
                self._send_json_response(ERR_INVALID_CONTENT_LENGTH)
                return

            # 2. 读取请求体
            try:
                post_data_bytes = self.rfile.read(content_length)
                post_data_string = post_data_bytes.decode("utf-8")  # 假设 UTF-8
            except Exception as e:
                print(f"读取请求体时出错: {e}")
                self._send_json_response(ERR_READ_BODY_FAILED)
                return

            # 3. 解析 JSON
            try:
                request_data = json.loads(post_data_string)
                print("请求体 JSON 数据:")
                print(json.dumps(request_data, indent=4, ensure_ascii=False))

                # --- 核心处理逻辑 (当前只是打印) ---
                # 在这里可以添加实际的处理 request_data 的代码
                # 如果处理过程中出现特定错误，可以定义新的错误码并调用 _send_json_response

                # 4. 处理成功
                self._send_json_response(ERR_SUCCESS)

            except json.JSONDecodeError:
                self._send_json_response(ERR_INVALID_JSON)
            except Exception as e:
                # 捕获解析后、发送响应前可能发生的其他意外错误
                print(f"处理请求数据时出错: {e}")
                self._send_json_response(ERR_PROCESSING_FAILED)

        else:
            # 路径不匹配
            self._send_json_response(ERR_NOT_FOUND)

    def do_GET(self):
        """处理 GET 请求，始终返回 JSON 错误响应。"""
        parsed_path = urlparse(self.path)
        path = parsed_path.path

        if path == "/get_response":
            # 不允许 GET 访问此端点
            self._send_json_response(ERR_METHOD_NOT_ALLOWED)
        else:
            # 其他路径不存在
            self._send_json_response(ERR_NOT_FOUND)

    # 可以为其他方法 (PUT, DELETE 等) 添加类似 do_GET 的处理
    def do_PUT(self):
        self._send_json_response(ERR_METHOD_NOT_ALLOWED)

    def do_DELETE(self):
        self._send_json_response(ERR_METHOD_NOT_ALLOWED)


# --- 启动服务器 ---
Handler = SimpleHTTPRequestHandler
httpd = socketserver.TCPServer((HOST, PORT), Handler)

print(f"HTTP 服务器正在监听 http://{HOST}:{PORT}/")
print("现在处理 /get_response 的 POST 请求，并始终返回 JSON 响应。")
print("错误码定义见代码注释或文档。")
print("按 Ctrl+C 停止服务器")

try:
    httpd.serve_forever()
except KeyboardInterrupt:
    print("\n收到停止信号，正在关闭服务器...")
    httpd.server_close()
    print("服务器已关闭。")
