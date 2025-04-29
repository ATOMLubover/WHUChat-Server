from openai import OpenAI
def chatgpt_chat(model="gemini-2.5-flash-preview-04-17", messages=None, temperature=0.7):
    client = OpenAI(api_key="AIzaSyDfsvYh5Okgo-qQEyaLNZZLAoLnI9jkbMg",
                    base_url="https://generativelanguage.googleapis.com/v1beta/openai/") 
    messages1 = [{"role": "system", "content": "请详细展示推理过程，然后给出最终回答，格式如下：Reasoning: ... Contents: ..."}] + (messages or [])
    response = client.chat.completions.create(
        model=model,
        messages=messages1,
        stream=True,
        temperature=temperature
    )
    current_type = None
    for chunk in response:
        delta_content = chunk.choices[0].delta.content
        if delta_content:
            if delta_content.strip().startswith("Reasoning:"):
                current_type = "reasoning"
                yield {"type": "reasoning", "reasoning_content": delta_content.strip()[len("Reasoning:"):].strip()}
            elif delta_content.strip().startswith("Contents:"):
                current_type = "content"
                yield {"type": "content", "content": delta_content.strip()[len("Contents:"):].strip()}
            else:
                if current_type == "reasoning":
                    yield {"type": "reasoning", "reasoning_content": delta_content}
                elif current_type == "content":
                    yield {"type": "content", "content": delta_content}

if __name__ == "__main__":
    prompt = "Explain the concept of Occam's Razor and provide a simple, everyday example."
    messages = [{"role": "user", "content": prompt}]
    
    # 调用 chatgpt_chat
    for chunk in chatgpt_chat(messages=messages):
        print("receive")
        print(chunk)  # 打印每个 chunk 的内容
        if chunk["type"] == "reasoning":
            print(f"[Reasoning] {chunk['reasoning_content']}")
        elif chunk["type"] == "content":
            print(f"[Content] {chunk['content']}")