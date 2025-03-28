#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include "hd_logger.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    HD_LOGGER_INFO("hdshell",">>> Shell service handle_signal (PID: %d) sig=%d\n", getpid(),sig);
    running = 0;
}

int main() {
    HD_LOGGER_INFO("hdshell",">>> Shell service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    int index = 0;
    // 日志服务逻辑
    while (running) {
        time_t now = time(NULL);
        HD_LOGGER_INFO("hdshell",">>> Shell service heartbeat:%d\n", index++);
        sleep(4);
    }
    
    HD_LOGGER_INFO("hdshell",">>> Shell service stopped !!!\n");
    return 0;
}