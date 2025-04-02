import os
from openai import OpenAI
def get_chat_completion(api_key,base_url,model, messages):
    client = OpenAI(
        api_key=api_key,
        base_url=base_url
        
    )
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
        stream=True
    )
    for chunk in completion:
        if chunk.choices[0].delta.content is not None:
            yield(chunk.choices[0].delta.content)