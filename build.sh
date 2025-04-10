#!/bin/bash

# Build script for hdinit project
# Usage: ./build.sh

# Function to check if compilation succeeded
check_compile() {
    if [ $? -ne 0 ]; then
        echo "Error: Compilation failed for $1"
        exit 1
    fi
    echo "Successfully compiled $1"
}

# Create .service directory if it doesn't exist
mkdir -p ./.service

echo "Compiling hd_init (standard output)..."
gcc -o ./.service/hd_init hd_init.c hd_logger.c hd_utils.c hd_service.c hd_ipc.c -lpthread hd_http.c ./cJSON.c -lcurl
check_compile "hd_init (standard)"

echo "Compiling hdshell (service version)..."
gcc hd_shell.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -lpthread -o ./.service/hdshell
check_compile "hdshell (service)"

echo "Compiling hdmain..."
gcc hd_main.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -o ./.service/hdmain
check_compile "hdmain"

echo "Compiling hdlog..."
gcc hd_log.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -o ./.service/hdlog
check_compile "hdlog"

echo ""
echo "All compilations completed successfully"
echo "Output files:"
echo "  ./hdshell"
echo "  ./.service/hdshell"
echo "  ./.service/hdmain"
echo "  ./.service/hdlog"
