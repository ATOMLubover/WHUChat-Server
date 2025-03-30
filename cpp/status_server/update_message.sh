#!/bin/bash

# 定义路径等变量
PROTO_DIR="./resources/config/"
PROTO_FILE="./resources/config/message.proto"
OUTPUT_DIR="./sources/thirdparty"
CURR_DIR=$(pwd)
GRPC_PLUGIN_PATH=$(which grpc_cpp_plugin)

# 检查 proto 文件是否存在
if [ ! -f "$PROTO_FILE" ]; then
    echo "Error: proto file not found at $PROTO_FILE"
    exit 1
fi

# 检查输出目录是否存在
if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Error: output dir not found at $OUTPUT_DIR"
    exit 1
fi

# 检查 grpc_cpp_plugin 是否存在
if [ -z "$GRPC_PLUGIN_PATH" ]; then
    echo "Error: grpc_cpp_plugin not found. Make sure gRPC is installed."
    exit 1
fi

# 获得原文件名
copied_file=$(basename "$PROTO_FILE")
# 由于 protoc 的神笔寻路方式，这里进行文件的现场转移
echo "Copiing proto file from $PROTO_DIR to $CURR_DIR"
cp "$PROTO_FILE" ./"$copied_file"
if [ $? -ne 0 ]; then
    echo "Error: unabe to copy proto file"
    exit 1
fi

# 执行编译命令
echo "Compiling proto file..."
protoc --cpp_out=. \
       --grpc_out=. \
       --plugin=protoc-gen-grpc="$GRPC_PLUGIN_PATH" \
       ./"$copied_file"

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "Proto compilation successful!"
    echo "Generated files in $OUTPUT_DIR:"
    ls -l "$OUTPUT_DIR" | grep -E 'message\.pb|message\.grpc\.pb'
else
    echo "Proto compilation failed!"
    exit 1
fi

# 剪切指定文件到目标目录
echo "Moving .pb.cc and .pb.h files to $OUTPUT_DIR..."
mv *.pb.cc *.pb.h "$OUTPUT_DIR"
if [ $? -ne 0 ]; then
    echo "Error occurred during move operation."
    # 即使出错也要尝试删除已复制的文件
    rm -f "$copied_file"
    exit 1
fi

# 删除复制的 proto 文件
echo "Deleting copied file $copied_file..."
rm -f "$copied_file"
if [ $? -ne 0 ]; then
    echo "Warning: Failed to delete copied file $copied_file"
fi