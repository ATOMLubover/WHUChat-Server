from typing import *
 
import os
import json
 
from openai import OpenAI
from openai.types.chat.chat_completion import Choice
api_key = "sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA"
base_url="https://api.moonshot.cn/v1",
def search_impl(arguments: Dict[str, Any]) -> Any:
    """
    在使用 Moonshot AI 提供的 search 工具的场合，只需要原封不动返回 arguments 即可，
    不需要额外的处理逻辑。
 
    但如果你想使用其他模型，并保留联网搜索的功能，那你只需要修改这里的实现（例如调用搜索
    和获取网页内容等），函数签名不变，依然是 work 的。
 
    这最大程度保证了兼容性，允许你在不同的模型间切换，并且不需要对代码有破坏性的修改。
    """
    return arguments
def kimi_chat(messages,temperature=0.7,enableWebSearch=False,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA", base_url="https://api.moonshot.cn/v1")
        if enableWebSearch:
            response = client.chat.completions.create(
            model="kimi-latest",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            tools=[
            {
                "type": "builtin_function",  # <-- 使用 builtin_function 声明 $web_search 函数，请在每次请求都完整地带上 tools 声明
                "function": {
                    "name": "$web_search",
                },
            }
            ]
            )
        else:
            response = client.chat.completions.create(
            model="kimi-latest",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return

def moonshot_chat(messages,temperature=0.7,enableWebSearch=False,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA", base_url="https://api.moonshot.cn/v1")
        if enableWebSearch:
            response = client.chat.completions.create(
            model="moonshot-v1-8k",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            tools=[
            {
                "type": "builtin_function",  # <-- 使用 builtin_function 声明 $web_search 函数，请在每次请求都完整地带上 tools 声明
                "function": {
                    "name": "$web_search",
                },
            }
            ]
            )
        else:
            response = client.chat.completions.create(
            model="moonshot-v1-8k",
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

def kimi_reasoner(messages,temperature=0.7,enableWebSearch=False,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA", base_url="https://api.moonshot.cn/v1")
        response = client.chat.completions.create(
            model="kimi-thinking-preview",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        thinking = False
        for chunk in response:
            if chunk.choices:
                choice = chunk.choices[0]
                # 由于 openai SDK 并不支持输出思考过程，也没有表示思考过程内容的字段，因此我们无法直接通过 .reasoning_content 获取自定义的表示 kimi 推理过程的
                # reasoning_content 字段，只能通过 hasattr 和 getattr 来间接获取该字段。
                #
                # 我们先通过 hasattr 判断当前输出内容是否包含 reasoning_content 字段，如果包含，再通过 getattr 取出该字段并打印。
                if choice.delta and hasattr(choice.delta, "reasoning_content"):
                    if not thinking:
                        thinking = True
                    #yield getattr(choice.delta, "reasoning_content")
                if choice.delta and choice.delta.content:
                    if thinking:
                        thinking = False
                    yield {"type": "content", "content": choice.delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return
    
if __name__ == "__main__":
    messages = [
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Hello!"}
    ]
    for chunk in kimi_chat(messages):
        print(chunk)
    for chunk in moonshot_chat(messages):
        print(chunk)
    for chunk in kimi_reasoner(messages):
        print(chunk)