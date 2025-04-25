#!/bin/bash

# 错误处理函数
handle_error() {
    echo "Error occurred at line $1, command: $2"
    exit 1
}

# 启用错误追踪
trap 'handle_error $LINENO "$BASH_COMMAND"' ERR

# 延时秒数
DELAY=1

# 进入lrpc目录编译
echo "==== Building lrpc ===="
cd lipc
make clean && make && sleep $DELAY
cd ..

# 进入mqtt_client目录编译并推送
echo "==== Building mqtt_client ===="
cd mqtt__client
make clean && make && sleep $DELAY
adb push mqtt_client /root/hdmqtt
cd ..

# 进入uart_client目录编译并推送
echo "==== Building uart_client ===="
cd uart__client
make clean && make && sleep $DELAY
adb push uart_client /root/hduart
cd ..

# 进入rpc_server目录编译并推送
echo "==== Building rpc_server ===="
cd rpc_server
make clean && make && sleep $DELAY
adb push rpc_server /root/hdrpc
cd ..

echo "==== All builds and pushes completed successfully ===="
