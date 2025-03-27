import os
from openai import OpenAI
def get_chat_completion(api_key,base_url,model, messages):
    client = OpenAI(
        base_url=base_url,
        api_key=api_key
    )
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
    )
    return completion.choices[0].message.content