import requests
from openai import OpenAI
# 你的 API Key
API_KEY = "sk-176d442796bf4b4f9cf28afdb03d25ae"  
base_url="https://api.deepseek.com"

def deepseek_chat(model="deepseek-chat", contents="Explain how AI works in a few words"):
    try:
        client = OpenAI(api_key=API_KEY, base_url=base_url)
        response = client.models.generate_content(
            model=model,
            contents=contents
        )
        return response.choices[0].message.content
    except Exception as e:
        print(f"错误信息：{e}")
        return None

user_input = {"role": "user", "content": "1+1等于多少"},
reply = deepseek_chat(user_input)
print("DeepSeek AI 回复:", reply)
