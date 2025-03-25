#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main() {
    printf("hdlog service started (PID: %d)\n", getpid());
    
    // 日志服务逻辑
    while (1) {
        time_t now = time(NULL);
        printf("[%s] Log service heartbeat\n", ctime(&now));
        sleep(30);
    }
    
    return 0;
}
