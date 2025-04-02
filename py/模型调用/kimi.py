from openai import OpenAI
api_key = "sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA"
base_url="https://api.moonshot.cn/v1",
def get_chat_completion(model, messages):
    client = OpenAI(
        api_key=api_key,
        base_url=base_url,
    )
    
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
        stream=True
    )
    
    for chunk in completion:
        if chunk.choices[0].delta.content is not None:
            yield(chunk.choices[0].delta.content)