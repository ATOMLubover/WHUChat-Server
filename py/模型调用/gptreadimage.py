
messages=[
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What are in these images? Is there any difference between them?"
        },
        {
          "type": "image_url",
          "image_url": {
            "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/d/dd/Gfp-wisconsin-madison-the-nature-boardwalk.jpg/2560px-Gfp-wisconsin-madison-the-nature-boardwalk.jpg"
          },
        },
        {
          "type": "image_url",
          "image_url": {
            "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/d/dd/Gfp-wisconsin-madison-the-nature-boardwalk.jpg/2560px-Gfp-wisconsin-madison-the-nature-boardwalk.jpg"
          },
        },
      ],
    }
  ],
import openai
from openai import OpenAI
def chatgpt_chat(messages=None):
    openai.api_key = "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"
    response = openai.chat.completions.create(
    model="gpt-4o",
    messages=messages,
    stream=True
    )
    for chunk in response:
        if chunk.choices[0].delta.content is not None:
            yield(chunk.choices[0].delta.content)

if __name__ == "__main__":
    for chunk in chatgpt_chat(messages):
        print(chunk,end="")
    print()
    print("done")