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

echo "所有编译完成"
