import openai
from openai import OpenAI
api_key="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"
def chatgpt_chat(model="gpt-3.5-turbo",messages=None):
    try:
        client = OpenAI(api_key)
        response = client.chat.completions.create(
        model=model,
        messages=messages,
        stream=True
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield(chunk.choices[0].delta.content)
    except openai.APIError as e:
        return(f"OpenAI API 调用出错: {e}")
    except openai.AuthenticationError as e:
        return(f"身份验证错误，请检查 API 密钥: {e}")
    except openai.RateLimitError as e:
        return(f"请求速率超出限制: {e}")
    except Exception as e:
        return(f"发生未知错误: {e}")