from openai import OpenAI
api_key = "sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA"
base_url="https://api.moonshot.cn/v1",
def get_chat_completion(api_key, model, messages, temperature=0.3):
    client = OpenAI(
        api_key=api_key,
        base_url=base_url,
    )
    
    completion = client.chat.completions.create(
        model=model,
        messages=messages,
        temperature=temperature,
    )
    
    return completion.choices[0].message.content