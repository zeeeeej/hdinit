# 编译器
CC = g++

# 编译选项
CFLAGS = -I/Users/xiangpengle/Documents/linux/code/hdinit/lib_service
LDFLAGS = -L/Users/xiangpengle/Documents/linux/code/hdinit/libs
LIBS = -lhsservice

# 源文件目录和文件
SRC_DIR = ..
SRCS = $(SRC_DIR)/HSServiceList.cpp test.cpp

# 目标文件目录和文件
BUILD_DIR = build
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 可执行文件
TARGET = $(BUILD_DIR)/test

# 默认目标
all: $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) ${CFLAGS} -std=c++11 -o $@ $^ $(LIBS)

# 编译源文件到目标目录
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR)

# 伪目标
.PHONY: all clean
