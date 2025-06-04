import os
from openai import OpenAI
api_key="sk-354859a6d3ae438fb8ab9b98194f5266"
base_url="https://dashscope.aliyuncs.com/compatible-mode/v1"
def tongyi_chat(messages=None,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:        
        client = OpenAI(
            api_key="sk-354859a6d3ae438fb8ab9b98194f5266",
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model="qwen-plus",
            messages=messages,
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        for chunk in completion:
            yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return None
    
def tongyi_mutichat(messages=None, temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    client = OpenAI(
        # 若没有配置环境变量，请用百炼API Key将下行替换为：api_key="sk-xxx",
        api_key=api_key,
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
def tongyi_gate(messages=None,temperature=0.7,model="qwen-plus"):
    match model:
        case "qwq-32b":
            return tongyi_reasoner(messages,temperature)
        case _:
            return tongyi_chat(model,messages,temperature)
        
def tongyi_reasoner(messages=None, temperature=0.7, model="qwq-32b", api_key=None, base_url=None,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    api_key = api_key or os.getenv("DASHSCOPE_API_KEY")
    base_url = base_url or "https://dashscope.aliyuncs.com/compatible-mode/v1"

    try:
        client = OpenAI(
            api_key="sk-354859a6d3ae438fb8ab9b98194f5266",
            base_url=base_url,
        )

        completion = client.chat.completions.create(
            model=model,
            messages=messages,
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )

        for chunk in completion:
            if not chunk.choices:
                continue
            delta = chunk.choices[0].delta

            if hasattr(delta, "reasoning_content") and delta.reasoning_content is not None:
                # yield {
                #     "type": "reasoning",
                #     "reasoning_content": delta.reasoning_content
                # }
                12222
            elif hasattr(delta, "content") and delta.content:
                yield {
                    "type": "content",
                    "content": delta.content
                }

    except Exception as e:
        print(f"错误信息：{e}")
        print("请参考文档：https://help.aliyun.com/zh/model-studio/developer-reference/error-code")
        return
    

if __name__ == "__main__":
    # 测试代码
    messages = [
        {"role": "user", "content": "你好"},
        {"role": "assistant", "content": "你好，我是一个AI助手。有什么我可以帮助你的吗？"},
        {"role": "user", "content": "1$1等于几"},
        {"role": "assistant", "content": "3"},
        {"role": "user", "content": "2+2等于几"},
        {"role": "assistant", "content": "4"},
        {"role": "user", "content": "3+3等于几"},
        {"role": "assistant", "content": "6"},
        {"role": "user", "content": "4+4等于几"},
        {"role": "assistant", "content": "8"},
        {"role": "user", "content": "5+5等于几"},
        {"role": "assistant", "content": "10"},
        {"role": "user", "content": "6+6等于几"},
        {"role": "assistant", "content": "12"},
        {"role": "user", "content": "Fe-109是什么，以及1$1等于几"}
    ]
    print(api_key)
    for chunk in tongyi_reasoner(messages):
        print("返回值：", chunk)