import requests

# 你的 API Key
API_KEY = "sk-176d442796bf4b4f9cf28afdb03d25ae"  

def deepseek_chat( model="deepseek-reasoner",messages=None):

    url = "https://api.deepseek.com/chat/completions"
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}"
    }
    
    data = {
        "model": model,
        "messages": messages,
           # {"role": "system", "content": "你是一个专业的助手"},
           # {"role": "user", "content": prompt}
        
    }

    try:
        response = requests.post(url, headers=headers, json=data)
        response.raise_for_status()  # 检查 HTTP 请求是否成功
        result = response.json()
        return result['choices'][0]['message']['content']
    except requests.exceptions.RequestException as e:
        return f"请求失败：{e}"

# 调用示例
if __name__ == "__main__":
    user_input = "1+1等于多少？"
    reply = deepseek_chat(user_input)
    print("DeepSeek AI 回复:", reply)
