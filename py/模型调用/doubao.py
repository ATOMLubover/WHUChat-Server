import os
from openai import OpenAI
api_key="3c9c97eb-532c-48b9-acdc-110e8d0f98ea"
def get_chat_completion(model, messages,temperature=0.7):
    client = OpenAI(
        api_key=api_key,
        base_url="https://ark.cn-beijing.volces.com/api/v3/"
        
    )
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
        stream=True,
        temperature=temperature
    )
    for chunk in completion:
        if chunk.choices[0].delta.content is not None:
            yield(chunk.choices[0].delta.content)