#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
    printf("hdlog service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    int index = 0;
    // 日志服务逻辑
    while (running && (index < 5)) {
        time_t now = time(NULL);
        printf("Log service heartbeat:%d\n",index++);
        sleep(5);
    }
    
    printf("hdlog service stopped\n");
    return 0;
}