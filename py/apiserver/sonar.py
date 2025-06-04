import requests
import os
from openai import OpenAI

def sonar_chat(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="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", base_url="https://api.perplexity.ai")
        response = client.chat.completions.create(
            model="sonar",
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



def sonarpro(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="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", base_url="https://api.perplexity.ai")
        response = client.chat.completions.create(
            model="sonar-pro",
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
    {
        "role": "system",
        "content": (
            "You are an artificial intelligence assistant and you need to "
            "engage in a helpful, detailed, polite conversation with a user."
        ),
    },
    {   
        "role": "user",
        "content": (
            "How many stars are in the universe?"
        ),
    },
    ]
    for chunk in sonar_chat(messages):
        print(chunk)
    print("======================================")
    for chunk in sonarpro(messages):
        print(chunk)
