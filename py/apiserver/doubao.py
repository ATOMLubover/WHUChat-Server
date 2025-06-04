import os
from openai import OpenAI
baseurl = "https://ark.cn-beijing.volces.com/api/v3/"
api_key="2233b1d1-ac2d-4f94-afb4-ac2192568a76"
def doubao_completion(model="doubao-1-5-pro-32k-250115", messages=None,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(
            api_key="2233b1d1-ac2d-4f94-afb4-ac2192568a76",
            base_url="https://ark.cn-beijing.volces.com/api/v3/"
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
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return

def doubao_reasoner(model="doubao-1-5-thinking-pro-250415", messages=None,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(
            api_key="2233b1d1-ac2d-4f94-afb4-ac2192568a76",
            base_url="https://ark.cn-beijing.volces.com/api/v3/"
        )
        response = client.chat.completions.create(
            model=model,
            messages=messages,
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        for chunk in response:
            if chunk.choices:
                choice = chunk.choices[0]
                # 由于 openai SDK 并不支持输出思考过程，也没有表示思考过程内容的字段，因此我们无法直接通过 .reasoning_content 获取自定义的表示 kimi 推理过程的
                # reasoning_content 字段，只能通过 hasattr 和 getattr 来间接获取该字段。
                #
                # 我们先通过 hasattr 判断当前输出内容是否包含 reasoning_content 字段，如果包含，再通过 getattr 取出该字段并打印。
                if choice.delta and hasattr(choice.delta, "reasoning_content"):
                    # if not thinking:
                    #     thinking = True
                    # yield getattr(choice.delta, "reasoning_content")
                    111
                if choice.delta and choice.delta.content:
                    yield {"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return
    
if __name__ == "__main__":
    messages = [
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Who won the world series in 2020?"},
    ]
    for chunk in doubao_completion(messages=messages,temperature=0.7):
        print(chunk)
    for chunk in doubao_reasoner(messages=messages,temperature=0.7):
        print(chunk)
