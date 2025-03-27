#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
    printf("hdmain service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    
    // 主业务逻辑
    while (running) {
        sleep(10);
        printf("hdmain is running...\n");
    }
    
    printf("hdmain service stopped\n");
    return 0;
}
