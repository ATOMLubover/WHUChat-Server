from openai import OpenAI

API_KEY = "AIzaSyDfsvYh5Okgo-qQEyaLNZZLAoLnI9jkbMg"

def gemini_chat(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="AIzaSyDfsvYh5Okgo-qQEyaLNZZLAoLnI9jkbMg", base_url="https://generativelanguage.googleapis.com/v1beta/openai/")
        response = client.chat.completions.create(
            model="gemini-2.0-flash",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return

if __name__ == "__main__":
    messages = [
        {"role": "user", "content": "你好"},
    ]
    for chunk in gemini_chat(messages):
        print(chunk)