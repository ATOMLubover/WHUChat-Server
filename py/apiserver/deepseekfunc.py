import requests
import os
from openai import OpenAI
# 你的 API Key
API_KEY = os.getenv("DEEPSEEK_API_KEY", "sk-176d442796bf4b4f9cf28afdb03d25ae")  # 默认值仅用于开发测试
BASE_URL = os.getenv("DEEPSEEK_BASE_URL", "https://api.deepseek.com")
def deepseek_chat(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="sk-176d442796bf4b4f9cf28afdb03d25ae", base_url="https://api.deepseek.com")
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        for chunk in response:
            if chunk.choices[0].delta.content is not None:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return

def convert_promote_to_message(promote):
    message = []
    for item in promote:
        role = item.get("role")
        contents = item.get("content", [])
        # 只提取所有 type == 'text' 的 text 字段并拼接
        text = "".join(c.get("text", "") for c in contents if c.get("type") == "text")
        message.append({"role": role, "content": text})
    return message

def deepseek_chatreasoner(messages,temperature=0.7,max_tokens=1024,presence_penalty=0.0,top_p=1.0):
    try:
        client = OpenAI(api_key="sk-176d442796bf4b4f9cf28afdb03d25ae", base_url="https://api.deepseek.com")
        response = client.chat.completions.create(
            model="deepseek-reasoner",
            messages=messages,  # 使用 messages 参数
            stream=True,
            temperature=temperature,
            max_tokens=max_tokens,
            presence_penalty=presence_penalty,
            top_p=top_p
        )
        for chunk in response:
            if chunk.choices[0].delta.reasoning_content is not None:
                #yield{"type": "reasoning", "reasoning_content": chunk.choices[0].delta}
                12222
            else:
                yield{"type": "content", "content": chunk.choices[0].delta.content}
    except Exception as e:
        print(f"错误信息：{e}")
        return


if __name__ == "__main__":
    # messages = [
    #     {"role": "system", "content": "You are a helpful assistant."},
    #     {"role": "user", "content": "Hello!"}
    # ]
    messages = [
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What’s your name?"
        },
        {
          "type": "text",
          "text": "How are you?"
        }
      ],
    }
  ]
    messages2=[{"role": "user", 
                "content": [
                    {"type": "text", "text": "b站是什么"}
                ]
                }, 
                {"role": "assistant", 
                 "content": [
                     {"type": "text", "text": "B站是中国大陆知名的视频分享平台，正式名称为**哔哩哔哩（Bilibili）**，简称B站。以下是关于B站的关键信息：\n\n### 1. **基本概况**\n   - **成立时间**：2009年，最初以二次元（ACG）内容为核心。\n   - **用户群体**：主要面向年轻用户（Z世代），以弹幕互动文化著称。\n   - **特色功能**：弹幕（实时评论）、分区内容（动画、游戏、鬼畜、知识等）、UP主创作生态。\n\n### 2. **主要内容**\n   - **ACG文化**：动画、漫画、游戏（尤其是日本番剧、国产动漫）。\n   - **多元化扩展**：涵盖科技、生活、时尚、学习（如“知识区”）、影视解说等。\n   - **原创内容**：大量UP主自制视频，包括评测、Vlog、教程等。\n\n### 3. **商业模式**\n   - **大会员**：付费订阅制，提供高清、独家内容。\n   - **直播与电商**：虚拟主播、游戏直播、周边商品售卖。\n   - **广告与联运游戏**：游戏代理（如《FGO》）是重要收入来源。\n\n### 4. **文化影响**\n   - **弹幕文化**：用户通过弹幕实时互动，形成独特的社区氛围。\n   - **网络流行语发源地**：如“awsl”“爷青回”“一键三连”等均源自B站。\n   - **跨年晚会**：B站晚会以“懂年轻人”出圈，成为现象级活动。\n\n### 5. **相关数据**\n   - **月活用户**：超3亿（2023年数据）。\n   - **纳斯达克上市**：2018年在美国上市，2021年在香港二次上市。\n\n### 6. **注意事项**\n   - **内容审核**：受中国监管要求，部分海外内容可能受限。\n   - **国际版**：有面向海外的“Bilibili International”（内容较少）。\n\n如果你对B站的某个具体方面（如如何使用、热门UP主推荐等）感兴趣，可以进一步提问！"}]}, 
                {"role": "user", 
                 "content": [{"type": "text", "text": "它相比于传统视频平台有什么优势"}]}]
    messages3=convert_promote_to_message(messages2)
    print(API_KEY)
    result = {"reasoning_content": "", "content": ""}
    for chunk in deepseek_chat(messages3, 1.0,1024,0.0,1.0):
        print("返回值：", chunk)
"""         if chunk["type"] == "reasoning":
            result["reasoning_content"] += chunk["reasoning_content"]
        elif chunk["type"] == "content":
            result["content"] += chunk["content"]

    print("\n最终合并结果：")
    print(result) """