import os
from openai import OpenAI
api_key="3c9c97eb-532c-48b9-acdc-110e8d0f98ea"
def get_chat_completion(model, messages):
    client = OpenAI(
        api_key=api_key,
        base_url="https://ark.cn-beijing.volces.com/api/v3/"
        
    )
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
    )
    return completion.choices[0].message.content