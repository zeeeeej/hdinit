# Compiler and flags
CC = gcc
CFLAGS = -I./includes -Wall -Wextra
LDFLAGS = -L./libs -lcurl -ljsonrpcc -lev -lhdsafemap -lpthread

# Source directories and files
SRC_DIR = .
SERVICE_DIR = ./.service

# Targets
TARGETS = $(SERVICE_DIR)/hd_init $(SERVICE_DIR)/hdlog $(SERVICE_DIR)/hdmain

# Common source files used by multiple targets
COMMON_SRCS = hd_logger.c hd_utils.c hd_ipc.c cJSON.c hd_ipc_protocol.c

# Individual target sources
HD_INIT_SRCS = hd_init.c hd_ipc_init.c hd_service.c hd_http.c
HDLOG_SRCS = hd_log.c hd_ipc_service.c hd_ipc_client.c
HDMAIN_SRCS = hd_main.c hd_ipc_service.c hd_ipc_client.c

# Object files
HD_INIT_OBJS = $(addprefix $(SRC_DIR)/, $(COMMON_SRCS:.c=.o) $(HD_INIT_SRCS:.c=.o))
HDLOG_OBJS = $(addprefix $(SRC_DIR)/, $(COMMON_SRCS:.c=.o) $(HDLOG_SRCS:.c=.o))
HDMAIN_OBJS = $(addprefix $(SRC_DIR)/, $(COMMON_SRCS:.c=.o) $(HDMAIN_SRCS:.c=.o))

# Default target
all: $(TARGETS)

# Build rules
$(SERVICE_DIR)/hd_init: $(HD_INIT_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVICE_DIR)/hdlog: $(HDLOG_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVICE_DIR)/hdmain: $(HDMAIN_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(HD_INIT_OBJS) $(HDLOG_OBJS) $(HDMAIN_OBJS) $(TARGETS)

.PHONY: all clean
