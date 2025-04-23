# 编译器
CC = gcc

# 编译选项
# CFLAGS = -I/usr/local/imx6ull/curl/include -I/usr/local/imx6ull/ssl/include
# LDFLAGS = -L/usr/local/imx6ull/curl/lib -L/usr/local/imx6ull/ssl/lib
LIBS = -lpthread

# 源文件目录和文件
SRC_DIR = .
SRCS = $(SRC_DIR)/hd_safe_map.c

# 目标文件目录和文件
BUILD_DIR = build
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 可执行文件
TARGET = $(SRC_DIR)/../libs/libhdsafemap.so

# 默认目标
all: $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -shared -fPIC -o $@ $^ $(LIBS)

# 编译源文件到目标目录
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR)

# 伪目标
.PHONY: all clean
