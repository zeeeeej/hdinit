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
    printf("hdshell service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    
    // 日志服务逻辑
    while (running) {
        time_t now = time(NULL);
        printf("Shell service heartbeat\n");
        sleep(10);
    }
    
    printf("hdshell service stopped\n");
    return 0;
}