from google import genai
api_key="AIzaSyDfsvYh5Okgo-qQEyaLNZZLAoLnI9jkbMg"

def gemini_chat(model="gemini-2.0-flash", contents="Explain how AI works in a few words"):
    try:
        client = genai.Client(api_key=api_key)
        response = client.models.generate_content(
            model=model,
            contents=contents
        )
        return response.text
    except Exception as e:
        print(f"错误信息：{e}")
        return None
