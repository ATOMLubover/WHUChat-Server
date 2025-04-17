from google import genai

API_KEY = "AIzaSyDfsvYh5Okgo-qQEyaLNZZLAoLnI9jkbMg"

def generate_content_stream(model, contents,temperature=0.7):

    client = genai.Client(api_key=API_KEY)
    response = client.models.generate_content_stream(model=model, contents=contents)
    
    for chunk in response:
        yield {"type": "content", "content": chunk.text}

