#!/bin/bash

# 检查 adb 是否可用
if ! command -v adb &> /dev/null; then
    echo "错误: adb 未找到，请安装 Android SDK 或配置 PATH"
    exit 1
fi

# 检查设备是否连接
if ! adb devices | grep -q "device$"; then
    echo "错误: 未检测到已连接的 Android 设备"
    exit 1
fi

# 定义要推送的文件列表
files=(
    "./_hdinit/build/hd_init"
    "./_hdmain/build/hdmain"
    "./_hdlog/build/hdlog"
    "./_hdshell/build/hdshell"
)

# 检查所有文件是否存在
missing_files=()
for file in "${files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_files+=("$file")
    fi
done

if [ ${#missing_files[@]} -gt 0 ]; then
    echo "错误: 以下文件未找到:"
    for file in "${missing_files[@]}"; do
        echo "  - $file"
    done
    echo "请先编译项目或检查路径"
    exit 1
fi

# 执行推送
echo "正在推送文件到设备的 /root 目录..."
for file in "${files[@]}"; do
    adb push "$file" /root/
    if [ $? -ne 0 ]; then
        echo "警告: 推送 $file 失败"
    else
        echo "已推送: $file"
    fi
done

echo "推送完成"

# 可选: 设置文件可执行权限
echo "设置可执行权限..."
for file in "${files[@]}"; do
    filename=$(basename "$file")
    adb shell chmod +x "/root/$filename"
done

echo "操作完成"
