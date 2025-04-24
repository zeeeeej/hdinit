#!/bin/bash

echo 进入_hdinit目录并编译
cd ./_hdinit/
make clean
make

echo 进入_hdmain目录并编译
cd ../_hdmain/
make clean
make

echo 进入_hdlog目录并编译
cd ../_hdlog/
make clean
make

echo 进入_hdshell目录并编译
cd ../_hdshell/
make clean
make


echo 进入rpc_server目录并编译
cd ..
cp ./hd_service_interface.* ./hd_utils.* ./hd_logger.* ./hd_ipc.* /home/book/RPC_LED_TEMP_HUMI_SERVER/
cd /home/book/RPC_LED_TEMP_HUMI_SERVER/
make clean
make

echo "所有编译完成"
