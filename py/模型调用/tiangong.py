import requests
import time
import hashlib
import json

app_key="c5fe2382ec8bfef3f625d3dec8f3bf75"
app_secret="c5fe2382ec8bfef3f625d3dec8f3bf75"
def doubao_stream_chat(messages,temperature):
    url = 'https://api-maas.singularity-ai.com/sky-work/api/v1/chat'

    timestamp = str(int(time.time()))
    sign_content = app_key + app_secret + timestamp
    sign_result = hashlib.md5(sign_content.encode('utf-8')).hexdigest()

    headers = {
        "app_key": app_key,
        "timestamp": timestamp,
        "sign": sign_result,
        "Content-Type": "application/json",
    }

    data = {
        "messages": messages,
        "intent": "chat"  # 可设置为空字符串 "" 看你是否启用搜索增强
    }

    try:
        response = requests.post(url, headers=headers, json=data, stream=True,temperature=temperature)

        for line in response.iter_lines():
            if line:
                try:
                    obj = json.loads(line.decode('utf-8'))
                    content = obj.get("data", {}).get("content")
                    if content:
                        yield {"type": "content", "content": content}
                except Exception as e:
                    print(f"解析流数据失败: {e}")
    except Exception as e:
        print(f"请求失败: {e}")
