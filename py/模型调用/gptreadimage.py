import openai
from openai import OpenAI
def chatgpt_chat(messages=None,temperature=0.7):
    openai.api_key = "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"
    response = openai.chat.completions.create(
    model="gpt-4o",
    messages=messages,
    stream=True,
    temperature=temperature
    )
    for chunk in response:
        if chunk.choices[0].delta.content is not None:
            yield(chunk.choices[0].delta.content)
