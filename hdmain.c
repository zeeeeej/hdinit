#include <stdio.h>
#include <unistd.h>

int main() {
    printf("hdmain service started (PID: %d)\n", getpid());
    
    // 主业务逻辑
    while (1) {
        sleep(10);
        printf("hdmain is running...\n");
    }
    
    return 0;
}
