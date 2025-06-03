import os
from openai import OpenAI
def get_chat_completion(api_key,base_url,model_type, promotes,temperature=0.7):
    client = OpenAI(
        api_key="sk-176d442796bf4b4f9cf28afdb03d25ae",
        base_url="https://api.deepseek.com"
    )
    completion = client.chat.completions.create(
        model=model_type,
        messages=promotes,
        stream=True,
        temperature=temperature
    )
    for chunk in completion:
        yield{"type": "content", "content": chunk.choices[0].delta.content}