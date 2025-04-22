import base64
import requests

def encode_image(image_path):
  with open(image_path, "rb") as image_file:
    return base64.b64encode(image_file.read()).decode('utf-8')
# Getting the base64 string
def get_base64_string(image_path):
  base64_string = encode_image(image_path)
  return base64_string

