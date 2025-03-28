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
    )
    return completion.choices[0].message.content