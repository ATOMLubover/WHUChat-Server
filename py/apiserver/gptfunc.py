import openai
from openai import OpenAI
api_key="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"

def chatgpt_chatreasoning(model="o1-mini-2024-09-12", messages=None, temperature=1.0,enableWebSearch=False):
    try:
        
        client = OpenAI(api_key="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")
        messages1 = (messages or [])
        if enableWebSearch:    
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=1.0,
                tools=[{"type": "web_search_preview"}]
            )
        else:
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=1.0
            )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except openai.APIError as e:
        return f"OpenAI API 调用出错: {e}"
    except openai.AuthenticationError as e:
        return f"身份验证错误，请检查 API 密钥: {e}"
    except openai.RateLimitError as e:
        return f"请求速率超出限制: {e}"
    except Exception as e:
        return f"发生未知错误: {e}"

def chatgpt_chat4(model="gpt-4o-2024-11-20", messages=None, temperature=0.7,enableWebSearch=False):
    try:
        client = OpenAI(api_key="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")
        messages1 = [{"role": "system", "content": "你是一个有用的助手"}] + (messages or [])
        if enableWebSearch:    
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=temperature,
                tools=[{"type": "web_search_preview"}]
            )
        else:
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=temperature
            )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except openai.APIError as e:
        return f"OpenAI API 调用出错: {e}"
    except openai.AuthenticationError as e:
        return f"身份验证错误，请检查 API 密钥: {e}"
    except openai.RateLimitError as e:
        return f"请求速率超出限制: {e}"
    except Exception as e:
        return f"发生未知错误: {e}"

def chatgpt_chat3(model="gpt-3.5-turbo-0125", messages=None, temperature=0.7,enableWebSearch=False):
    try:
        client = OpenAI(api_key="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")
        messages1 = [{"role": "system", "content": "你是一个有用的助手"}] + (messages or [])
        if enableWebSearch:    
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=temperature,
                tools=[{"type": "web_search_preview"}]
            )
        else:
            response = client.chat.completions.create(
                model=model,
                messages=messages1,
                stream=True,
                temperature=temperature
            )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except openai.APIError as e:
        return f"OpenAI API 调用出错: {e}"
    except openai.AuthenticationError as e:
        return f"身份验证错误，请检查 API 密钥: {e}"
    except openai.RateLimitError as e:
        return f"请求速率超出限制: {e}"
    except Exception as e:
        return f"发生未知错误: {e}"

if __name__ == "__main__":
    # 测试代码
    messages = [
        {"role": "user", "content": "你能告诉我今天的天气吗？"}
    ]
    for chunk in chatgpt_chatreasoning(messages=messages):
        print(chunk)
        print("end\n")
    for chunk in chatgpt_chat4(messages=messages):
        print(chunk)

