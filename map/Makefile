# 编译器
CC = arm-buildroot-linux-gnueabihf-gcc

# 编译选项
CFLAGS += -fPIC -Wall -Wextra  # 添加 -fPIC 和警告选项
LDFLAGS += -shared -fPIC -lpthread -lc  # 添加共享库和链接库选项

# 源文件目录和文件
SRC_DIR = .
SRCS = $(SRC_DIR)/hd_safe_map.c

# 目标文件目录和文件
BUILD_DIR = build
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 目标共享库
TARGET = ../libs/libhdsafemap.so

# 默认目标
all: $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 生成共享库
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# 编译源文件到目标文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 伪目标
.PHONY: all clean
