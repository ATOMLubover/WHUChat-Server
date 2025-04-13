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
            if chunk.choices[0].delta.content is not None:
                yield(chunk.choices[0].delta.content)
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