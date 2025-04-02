import requests

url = "http://127.0.0.1:8000"
data = {
    "model": "gemini-2.0-flash",
    "class": "gemini",
    "promote": ["Hello, how are you?"],
    "api_key": "your-api-key",
    "base_url": "your-base-url"
}

response = requests.post(url, json=data, stream=True)

for chunk in response.iter_content(None):
    if chunk:
        print(chunk.decode(), end="")