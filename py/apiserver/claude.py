import anthropic
api_key="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"
def stream_claude_response(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    client = anthropic.Anthropic(
        api_key=api_key
    )

    stream = client.messages.create(
        model="claude-3-7-sonnet-20250219",
        messages=messages,
        stream=True,
        max_tokens=max_tokens,
        presence_penalty=presence_penalty,
        top_p=top_p
    )

    for event in stream:
        if event.type == "content_block_delta":
            delta_text = event.delta.text
            if delta_text:
                yield {"type": "content", "content": delta_text}