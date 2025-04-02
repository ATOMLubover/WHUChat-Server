import requests
from openai import OpenAI

# 你的 API Key
API_KEY = "sk-176d442796bf4b4f9cf28afdb03d25ae"
base_url = "https://api.deepseek.com"

def deepseek_chat(model,messages):
    try:
        client = OpenAI(api_key=API_KEY, base_url=base_url)
        response = client.chat.completions.create(
            model=model,
            messages=messages,  # 使用 messages 参数
            stream=True
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield(chunk.choices[0].delta.content)
    except Exception as e:
        print(f"错误信息：{e}")
        return None