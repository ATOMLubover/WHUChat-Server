import asyncio
import json
import logging
import websockets
import gptreadimage
import deepseekfunc
import gptfunc
import tongyi
import gemini
import doubao
import default
import claude
# 服务器地址和端口
HOST = "127.0.0.1"
PORT = 8000

# 配置日志
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")

# WebSocket 处理逻辑
async def handle_websocket(websocket):
    logging.info("WebSocket 连接已建立")

    try:
        async for message in websocket:
            print(f"收到消息: {message}")
            data = json.loads(message)
            session_id = data.get("session_id", "未提供")
            talkdata = data.get("talkdata", {})
            model_type = talkdata.get("model", "未提供")
            model_class = talkdata.get("class", "未提供")
            promote_list = talkdata.get("promote", [])
            api_key = talkdata.get("api_key", "未提供")
            base_url = talkdata.get("base_url", "未提供")
            parameters = talkdata.get("parameters", {})
            temperature = parameters.get("temperature", 0.7)
            talktype = talkdata.get("type", "未提供")


            match talktype:
                case "word":            # 选择 AI 模型
                    match model_class:
                        case "deepseek":
                            result = deepseekfunc.deepseek_chat(model_type, promote_list, temperature)
                        case "chatgpt":
                            result = gptfunc.chatgpt_chat(model_type, promote_list,temperature)
                        case "tongyi":
                            result = tongyi.tongyi_chat(model_type, promote_list,temperature)
                        case "gemini":
                            result = gemini.gemini_chat(model_type, promote_list,temperature)
                        case "doubao":
                            result = doubao.get_chat_completion(model_type, promote_list,temperature)
                        case "claude":
                            result = claude.stream_claude_response(promote_list,temperature)
                        case _:
                            result = default.get_chat_completion(api_key, base_url, model_type, promote_list,temperature)
                        
                case "image":            # 选择 AI 模型
                    match model_class:
                        case "chatgpt":
                            result = gptreadimage.chatgpt_chat(model_type, promote_list,temperature)
                        case "qianwen":
                            result = tongyi.tongyi_mutichat(promote_list,temperature)

                case "audio":
                    match model_class:
                        case "qianwen":
                            result = tongyi.tongyi_mutichat(promote_list,temperature)
                    
                
                case "video":
                    match model_class:
                        case "qianwen":
                            result = tongyi.tongyi_mutichat(promote_list,temperature) 

            # 逐步发送流式响应
            for chunk in result:
                if isinstance(chunk, dict):
                    chunk_text = json.dumps(chunk)
                else:
                    chunk_text = str(chunk)
                await websocket.send(chunk_text)
            
            # 发送结束标识
            end_message = json.dumps({"end": True})
            await websocket.send(end_message)
            logging.info("数据流发送完毕，发送结束标识")

            # 服务器主动关闭连接
            await websocket.close()
            logging.info("WebSocket 连接已关闭")

    except websockets.exceptions.ConnectionClosed:
        logging.info("WebSocket 连接已关闭")
    except Exception as e:
        logging.error(f"WebSocket 发生错误: {e}")

# 运行 WebSocket 服务器
async def run_websocket_server():
    async with websockets.serve(handle_websocket, HOST, PORT):
        logging.info(f"WebSocket 服务器已启动：ws://{HOST}:{PORT}")
        await asyncio.Future()  # 让服务器保持运行

# 启动服务器
if __name__ == "__main__":
    try:
        asyncio.run(run_websocket_server())
    except KeyboardInterrupt:
        logging.info("服务器已关闭")
