import requests
import os
from openai import OpenAI
# 你的 API Key
API_KEY = os.getenv("DEEPSEEK_API_KEY", "sk-176d442796bf4b4f9cf28afdb03d25ae")  # 默认值仅用于开发测试
BASE_URL = os.getenv("DEEPSEEK_BASE_URL", "https://api.deepseek.com")
def deepseek_chat(messages,temperature=0.7):
    try:
        client = OpenAI(api_key="sk-176d442796bf4b4f9cf28afdb03d25ae", base_url="https://api.deepseek.com")
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return



def deepseek_chatreasoner(messages,temperature=0.7):
    try:
        client = OpenAI(api_key="sk-176d442796bf4b4f9cf28afdb03d25ae", base_url="https://api.deepseek.com")
        response = client.chat.completions.create(
            model="deepseek-reasoner",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature
        )
        for chunk in response:
            if chunk.choices[0].delta.reasoning_content is not None:
                #yield{"type": "reasoning", "reasoning_content": chunk.choices[0].delta}
                12222
            else:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return


if __name__ == "__main__":
    messages = [
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Hello!"}
    ]
    print(API_KEY)
    result = {"reasoning_content": "", "content": ""}
    for chunk in deepseek_chatreasoner(messages, 1.0):
        print("返回值：", chunk)
"""         if chunk["type"] == "reasoning":
            result["reasoning_content"] += chunk["reasoning_content"]
        elif chunk["type"] == "content":
            result["content"] += chunk["content"]

    print("\n最终合并结果：")
    print(result) """