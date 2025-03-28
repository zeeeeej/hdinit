#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include "hd_logger.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    HD_LOGGER_INFO("hdlog",">>> Log service handle_signal (PID: %d) sig=%d\n", getpid(),sig);
    running = 0;
}

int main() {
    HD_LOGGER_INFO("hdlog",">>> Log service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    //signal(SIGKILL, handle_signal);
    int index = 0;
    // 日志服务逻辑
    while (running) {
        time_t now = time(NULL);
        HD_LOGGER_INFO("hdlog",">>> Log service heartbeat:%d\n", index++);
        sleep(5);
    }
    
    HD_LOGGER_INFO("hdlog",">>> Log service stopped !!!\n");
    return 0;
}