import anthropic
api_key=""
def stream_claude_response(messgaes=None,model= "claude-3-7-sonnet-20250219"):
    client = anthropic.Anthropic(
        api_key=api_key
    )

    stream = client.messages.create(
        model=model,
        messages=messgaes,
        stream=True,
    )

    for event in stream:
        if event.type == "content_block_delta":
            print(event.delta.text, end="", flush=True)
