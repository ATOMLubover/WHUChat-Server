import openai
from openai import OpenAI
api_key="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"
def chatgpt_chat(messages="a white siamese cat"):
    try:
        client = OpenAI(api_key)
        response = client.images.generate(
        model="dall-e-3",
        prompt=messages,
        size="1024x1024",
        quality="standard",
        n=1,
        )
        image_url = response.data[0].url
        return image_url
    except openai.APIError as e:
        return(f"OpenAI API 调用出错: {e}")
    except openai.AuthenticationError as e:
        return(f"身份验证错误，请检查 API 密钥: {e}")
    except openai.RateLimitError as e:
        return(f"请求速率超出限制: {e}")
    except Exception as e:
        return(f"发生未知错误: {e}")