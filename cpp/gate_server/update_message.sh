#!/bin/bash

# 定义变量
PROTO_FILE="./resources/config/message.proto"
OUTPUT_DIR="./source/thirdparty/"
GRPC_PLUGIN_PATH=$(which grpc_cpp_plugin)

# 检查 proto 文件是否存在
if [ ! -f "$PROTO_FILE" ]; then
    echo "Error: Proto file not found at $PROTO_FILE"
    exit 1
fi

# 检查输出目录是否存在，不存在则创建
if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Creating output directory: $OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
fi

# 检查 grpc_cpp_plugin 是否存在
if [ -z "$GRPC_PLUGIN_PATH" ]; then
    echo "Error: grpc_cpp_plugin not found. Make sure gRPC is installed."
    exit 1
fi

# 执行编译命令
echo "Compiling $PROTO_FILE to $OUTPUT_DIR"
protoc --cpp_out="$OUTPUT_DIR" \
       --grpc_out="$OUTPUT_DIR" \
       --plugin=protoc-gen-grpc="$GRPC_PLUGIN_PATH" \
       "$PROTO_FILE"

# 检查是否成功
if [ $? -eq 0 ]; then
    echo "Proto compilation successful!"
    echo "Generated files in $OUTPUT_DIR:"
    ls -l "$OUTPUT_DIR" | grep -E 'message\.pb|message\.grpc\.pb'
else
    echo "Proto compilation failed!"
    exit 1
fi
