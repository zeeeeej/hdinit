
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
#include "hd_utils.h"

#define TAG "hdmain"
#define PREFIX "%%%%%%"
#define VERSION "0.0.3"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    HD_LOGGER_INFO(TAG,"%s %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% \n");
    HD_LOGGER_INFO(TAG,"%s Main service handle_signal (PID: %d) sig=%d\n", PREFIX,getpid(),sig);
    HD_LOGGER_INFO(TAG,"%s %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% \n");
    if (sig == SIGUSR1)
    {
      
    }
    else if (sig == SIGUSR2)
    {
        running=0;
    }
    else {
       
    }
    

}

/**
 * gcc  hd_main.c hd_logger.c hd_utils.c -o ./server/files/hdmain-0.0.3
 */
int main(int argc,const char *argv[]) {
    HD_LOGGER_INFO(TAG,"%s Main service started (PID: %d)\n",PREFIX, getpid());

    if (argc<=1)
    {
        HD_LOGGER_INFO(TAG,"%s Main service started return .\n",PREFIX);
        return 0 ;
    }
    
    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    /* 接受父进程的socked fd 进行通信 */
    int sock_fd = atoi(argv[1]);  // 获取父进程传递的 socket fd

    char buffer[HD_IPC_SOCKET_PATH_FOR_CHILD_BUFF_SIZE];
    // 返回给父进程表明启动成功 : <进程名称>,<进程id>,<程序版本号> 
    //sprintf(buffer,"%s,%d,%s","hdmain",getpid(),VERSION);

    const int  sid = getpid();

    hd_child_info_encode(buffer,"hdmain",sid,VERSION);

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