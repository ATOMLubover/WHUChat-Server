import os
from openai import OpenAI

api_key = "sk-c8vTitQmVcwwB7MF1bD0XkzDxqFxFEcm68XHBEkOD6E7FENA"

# 初始化OpenAI客户端
client = OpenAI(
    base_url="https://api.moonshot.cn/v1",
    api_key=api_key
)

try:
    # 获取模型列表并遍历
    models = client.models.list()  # 获取模型列表对象
    if hasattr(models, 'data'):  # 检查是否有'data'属性
        for m in models.data:  # 遍历模型列表
            print(m.id)
    else:
        print("返回的对象不包含模型列表，请检查API文档。")
except Exception as e:
    print(f"发生错误: {e}")