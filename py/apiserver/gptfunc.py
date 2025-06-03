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
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What’s your name?"
        },
        {
          "type": "text",
          "text": "How are you?"
        }
      ],
    }
  ]
    # for chunk in chatgpt_chatreasoning(messages=messages):
    #     print(chunk)
    #     print("end\n")
    for chunk in chatgpt_chat3(messages=messages):
        print(chunk)

