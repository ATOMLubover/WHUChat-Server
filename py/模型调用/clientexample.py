import asyncio
import websockets
import json

async def test_websocket():
    uri = "ws://127.0.0.1:8000"
    
    async with websockets.connect(uri) as websocket:
        # 发送请求
        request_data = {
            "model": "qwen-plus",
            "class": "tongyi",
            "promote": [ {"role": "system", "content": "你是一个 AI 助手"},{"role": "user", "content": "你好"}],
        }
        print("发送请求:", request_data)
        await websocket.send(json.dumps(request_data))
        
        # 逐步接收流式返回数据
        while True:
            response = await websocket.recv()
            print("服务器返回:", response)
            try:
                response_data = json.loads(response)
                if response_data.get("end"):
                    print("数据流接收完毕，发送结束标识")
                    break
            except json.JSONDecodeError:
                pass


asyncio.run(test_websocket())
