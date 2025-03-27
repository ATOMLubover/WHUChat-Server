import os
from openai import OpenAI
api_key="sk-354859a6d3ae438fb8ab9b98194f5266"
base_url="https://dashscope.aliyuncs.com/compatible-mode/v1"
def tongyi_chat( model="qwen-plus",messages=None):
    try:
        # 使用环境变量中的 API Key 或者传入的 API Key
        if api_key is None:
            api_key = os.getenv("DASHSCOPE_API_KEY")
        
        client = OpenAI(
            api_key=api_key,
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model=model,
            messages=messages
        )
        return completion.choices[0].message.content
    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return None

# 示例调用
#if __name__ == "__main__":
    messages = [
        {'role': 'system', 'content': 'You are a helpful assistant.'},
        {'role': 'user', 'content': '你是谁？'}
    ]
    response = get_chat_completion(messages=messages)
    if response:
        print(response)