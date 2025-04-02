import os
from openai import OpenAI
api_key="sk-354859a6d3ae438fb8ab9b98194f5266"
base_url="https://dashscope.aliyuncs.com/compatible-mode/v1"
def tongyi_chat(model="qwen-plus",messages=None):
    try:        
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
    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return None