import asyncio
import json
import logging
import aiohttp
from aiohttp import web
import websockets
import tiangong
import gptreadimage
import deepseekfunc
import gptfunc
import tongyi
import gemini
import doubao
import default
import claude

logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")
async def fetch_message_history(uuid: int, session_id: int | None):
    url = historyURL
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


async def process_and_send_to_wss(data):
    try:
        uuid = data.get("uuid")
        session_id = data.get("session_id")
        model_type = data.get("model")
        model_class = data.get("class")
        api_key = data.get("api_key")
        URL = data.get("URL")
        parameters = data.get("parameters", {})
        temperature = parameters.get("temperature", 0.7)
        talktype = parameters.get("type", "chat")

        history = await fetch_message_history(0, session_id)
        messages = history.get("messages")
        promote = [{"role": msg["role"], "content": msg["content"]} for msg in messages]

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

        reasoning_chunks = []
        content_chunks = []

        async with websockets.connect(wssURL) as websocket:
            logging.info(f"已连接到目标 WSS：{wssURL}")

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
                reasoning_message = {
                    "role": "reasoning",
                    "reasoning_content": "".join(reasoning_chunks)
                }
                await websocket.send(json.dumps(reasoning_message))
                logging.info("已发送 reasoning_content")

            if content_chunks:
                content_message = {
                    "role": "content",
                    "content": "".join(content_chunks)
                }
                await websocket.send(json.dumps(content_message))
                logging.info("已发送 content")

            # 结束标记
            await websocket.send(json.dumps({"end": True}))
            logging.info("发送 end 标志完成，关闭连接")

    except Exception as e:
        logging.error(f"WSS 推送过程中出错: {e}")



async def http_handler(request):
    try:
        # 尝试解析 JSON 请求体
        try:
            data = await request.json()
        except Exception as e:
            logging.error(f"请求体不是合法 JSON: {e}")
            return web.json_response({"errorcode": 3001, "message": "请求体必须为合法 JSON"})

        logging.info(f"接收到 HTTP 请求: {data}")

        # 检查必要字段是否存在
        required_fields = ["uuid", "session_id", "model", "class", "prompt"]
        missing_fields = [field for field in required_fields if field not in data]
        if missing_fields:
            message = f"缺少必要参数: {', '.join(missing_fields)}"
            logging.error(message)
            return web.json_response({"errorcode": 3002, "message": message})

        # 一切正常，异步处理任务
        asyncio.create_task(process_and_send_to_wss(data))
        return web.json_response({"errorcode": 0})

    except Exception as e:
        # 兜底异常处理
        logging.error(f"HTTP 处理过程中发生未知错误: {e}")
        return web.json_response({"errorcode": 3003, "message": f"内部错误: {str(e)}"})

    



async def main():
    with open("config.json", "r") as f:
        config = json.load(f)
    global historyURL, httpport, wssURL
    historyURL = config["database"]["historyURL"]
    httpport = config["database"]["httpport"]
    wssURL = config["database"]["wssURL"]
    app = web.Application()
    app.router.add_post("/", http_handler)
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, "0.0.0.0", httpport)
    await site.start()
    logging.info("HTTP 服务已启动")
    await asyncio.Future() 

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("服务已手动关闭")
