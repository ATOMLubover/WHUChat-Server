import requests
from openai import OpenAI
# 你的 API Key
API_KEY = "sk-176d442796bf4b4f9cf28afdb03d25ae"
base_url = "https://api.deepseek.com"

def deepseek_chat(messages,temperature=0.7):
    try:
        client = OpenAI(api_key=API_KEY, base_url=base_url)
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
        return None



def deepseek_chatreasoner(messages,temperature=0.7):
    try:
        client = OpenAI(api_key=API_KEY, base_url=base_url)
        response = client.chat.completions.create(
            model="deepseek-reasoner",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature
        )
        for chunk in response:
            if chunk.choices[0].delta.reasoning_content:
                yield{"type": "reasoning", "reasoning_content": chunk.choices[0].delta.reasoning_content}
            else:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return None
    

def deepseekgate(model,messages,temperature=0.7):
    match model:
        case "deepseek-chat":
            return deepseek_chat(messages,temperature)
        case "deepseek-reasoner":
            return deepseek_chatreasoner(messages,temperature)
        case _:
            return None




if __name__ == "__main__":
    messages = [
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Hello!"}
    ]
    for chunk in deepseek_chatreasoner(messages,1.0):
        print(chunk, end="")
    print()