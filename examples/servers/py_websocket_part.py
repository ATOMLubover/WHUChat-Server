import asyncio
import websockets
import signal

# 定义 WebSocket 服务器地址
WS_URL = "ws://127.0.0.1:8081/api/v1/ws/send_ans?token=api_server&session_id=8&uuid=1&model_id=1"


async def send_messages(ws):
    """从控制台读取输入并发送到 WebSocket 服务器"""
    try:
        while True:
            # 异步读取控制台输入（不阻塞事件循环）
            message = await asyncio.get_event_loop().run_in_executor(None, input)
            if message.strip().lower() == "exit":
                print("退出客户端...")
                await ws.close()
                break
            await ws.send(message)
    except asyncio.CancelledError:
        pass  # 正常退出


async def receive_messages(ws):
    """持续接收并打印服务器消息"""
    try:
        while True:
            message = await ws.recv()
            print(f"\n收到服务器消息: {message}")
    except websockets.exceptions.ConnectionClosed:
        print("连接已关闭")


async def listen_websocket():
    try:
        async with websockets.connect(WS_URL) as ws:
            print(f"已连接到 WebSocket 服务器: {WS_URL}")
            # 创建并并行运行发送和接收任务
            sender = asyncio.create_task(send_messages(ws))
            receiver = asyncio.create_task(receive_messages(ws))
            await asyncio.gather(sender, receiver)
    except Exception as e:
        print(f"连接异常: {e}")


def handle_exit(signal, frame):
    print("\n客户端已退出")
    exit(0)


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handle_exit)  # 注册 Ctrl+C 退出信号
    asyncio.get_event_loop().run_until_complete(listen_websocket())
