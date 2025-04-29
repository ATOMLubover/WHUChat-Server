import openai
from openai import OpenAI
api_key="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"

def chatgpt_chat(model="o4-mini", messages=None, temperature=0.7):
    try:
        client = OpenAI(api_key)
        messages1 = [{"role": "system", "content": "请详细展示推理过程，然后给出最终回答，格式如下：Reasoning: ... Contents: ..."}] + (messages or [])
        response = client.chat.completions.create(
            model=model,
            messages=messages1,
            stream=True,
            temperature=temperature
        )
        current_type = None
        for chunk in response:
            delta_content = chunk.choices[0].delta.get("content")
            if delta_content:
                if delta_content.strip().startswith("Reasoning:"):
                    current_type = "reasoning"
                    # 把前缀"Reasoning:"去掉，只要后面内容
                    yield {"type": "reasoning", "reasoning_content": delta_content.strip()[len("Reasoning:"):].strip()}
                elif delta_content.strip().startswith("Contents:"):
                    current_type = "content"
                    yield {"type": "content", "content": delta_content.strip()[len("Contents:"):].strip()}
                else:
                    # 继续上一次的type
                    if current_type == "reasoning":
                        yield {"type": "reasoning", "reasoning_content": delta_content}
                    elif current_type == "content":
                        yield {"type": "content", "content": delta_content}
    except openai.APIError as e:
        return f"OpenAI API 调用出错: {e}"
    except openai.AuthenticationError as e:
        return f"身份验证错误，请检查 API 密钥: {e}"
    except openai.RateLimitError as e:
        return f"请求速率超出限制: {e}"
    except Exception as e:
        return f"发生未知错误: {e}"
