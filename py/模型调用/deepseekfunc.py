import requests
from openai import OpenAI

# 你的 API Key
API_KEY = "sk-176d442796bf4b4f9cf28afdb03d25ae"
base_url = "https://api.deepseek.com"

def deepseek_chat(messages, model="deepseek-chat"):
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

# 正确构造 messages 参数
user_input = [
    {"role": "system", "content": "You are a helpful assistant"},
    {"role": "user", "content": "生成一篇1000字的作文"}
]

# 调用函数时，传递正确的参数
reply = deepseek_chat(user_input)  # 不需要再传递 model，默认值已设置
print("DeepSeek AI 回复:", reply)