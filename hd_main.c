
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include "hd_logger.h"
#include "hd_ipc.h"

#define TAG "hdmain"
#define PREFIX "%%%%%%"
#define VERSION "0.0.1"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    HD_LOGGER_INFO(TAG,"%sMain service handle_signal (PID: %d) sig=%d\n", PREFIX,getpid(),sig);
    if (sig == SIGUSR1)
    {
      
    }
    else {
        running=0;
    }
    

}

/**
 * gcc -o hdmain hd_main.c hd_logger.c
 */
int main(int argc,const char *argv[]) {
    HD_LOGGER_INFO(TAG,"%s Main service started (PID: %d)\n",PREFIX, getpid());

    signal(SIGUSR1, handle_signal);

    /* 接受父进程的socked fd 进行通信 */
    int sock_fd = atoi(argv[1]);  // 获取父进程传递的 socket fd

    char buffer[128];
    // 返回给父进程表明启动成功 : <进程名称>,<进程id>,<程序版本号> 
    sprintf(buffer,"%s,%d,%s","hdmain",getpid(),VERSION);

    write(sock_fd, buffer, strlen(buffer));

    close(sock_fd);
    /* 接受父进程的socked fd 进行通信  end*/

    int index = 0;
    
    while (running) {
        time_t now = time(NULL);
        HD_LOGGER_INFO(TAG,"%s Main service heartbeat:%d\n",PREFIX, index++);
        sleep(10);
    }
    
    HD_LOGGER_INFO(TAG,"%s Main service stopped !!!\n",PREFIX);
    return 0;
}