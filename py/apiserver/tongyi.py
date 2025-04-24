import os
from openai import OpenAI
api_key="sk-354859a6d3ae438fb8ab9b98194f5266"
base_url="https://dashscope.aliyuncs.com/compatible-mode/v1"
def tongyi_chat(model="qwen-plus",messages=None,temperature=0.7):
    try:        
        client = OpenAI(
            api_key=api_key,
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model=model,
            messages=messages,
            stream=True,
            temperature=temperature
        )
        for chunk in completion:
            if chunk.choices[0].delta.reasoning_content is not None:
                yield{"type": "reasoning", "reasoning_content": chunk.choices[0].delta.reasoning_content}
            else:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return None
    
def tongyi_mutichat(messages=None, temperature=0.7):
    client = OpenAI(
        # 若没有配置环境变量，请用百炼API Key将下行替换为：api_key="sk-xxx",
        api_key=os.getenv(api_key),
        base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
    )

    completion = client.chat.completions.create(
        model="qwen-omni-turbo",
        messages=messages,
        # 设置输出数据的模态，当前支持两种：["text","audio"]、["text"]
        modalities=["text", "audio"],
        audio={"voice": "Chelsie", "format": "wav"},
        temperature=temperature,
        # stream 必须设置为 True，否则会报错
        stream=True,
        stream_options={
            "include_usage": True
        }
    )

    for chunk in completion:
        if chunk.choices:
            print(chunk.choices[0].delta)
        else:
            print(chunk.usage)
def tongyi_reasoner(messages=None,temperature=0.7):
    try:        
        client = OpenAI(
            api_key=api_key,
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model="qwq-32b",
            messages=messages,
            stream=True,
            temperature=temperature
        )
        for chunk in completion:
            if chunk.choices[0].delta.reasoning_content is not None:
                yield{"type": "reasoning", "reasoning_content": chunk.choices[0].delta.reasoning_content}
            else:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return None
def tongyi_gate(messages=None,temperature=0.7,model="qwen-plus"):
    match model:
        case "qwq-32b":
            return tongyi_reasoner(messages,temperature)
        case _:
            return tongyi_chat(model,messages,temperature)
        
def tongyi_reasoner(messages=None, temperature=0.7, model="qwq-32b", api_key=None, base_url=None):
    api_key = api_key or os.getenv("DASHSCOPE_API_KEY")
    base_url = base_url or "https://dashscope.aliyuncs.com/compatible-mode/v1"

    try:
        client = OpenAI(
            api_key=api_key,
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model=model,
            messages=messages,
            stream=True,
            temperature=temperature
        )

        for chunk in completion:
            if not chunk.choices:
                continue
            delta = chunk.choices[0].delta

            if hasattr(delta, "reasoning_content") and delta.reasoning_content is not None:
                yield {
                    "type": "reasoning",
                    "reasoning_content": delta.reasoning_content
                }
            elif hasattr(delta, "content") and delta.content:
                yield {
                    "type": "content",
                    "content": delta.content
                }

    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return