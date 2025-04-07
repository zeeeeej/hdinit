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
#include "hd_utils.h"
#include "hd_ipc.h"
#include "hd_service_interface.h"


#define TAG "hdlog"
#define PREFIX ">>>>>>"
#define VERSION "1.0.1"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    HD_LOGGER_INFO(TAG,"%s Log service handle_signal (PID: %d) sig=%d\n", PREFIX,getpid(),sig);
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

void on_init (){

}

void on_start (){

}

void on_destory (){

}



/**
 * gcc -o hdlog hd_log.c hd_logger.c hd_utils.c
 * gcc  hd_log.c hd_logger.c hd_utils.c -o ./server/files/hdlog-1.0.2
 * 
 */
int main(int argc,const char *argv[]) {
    HD_LOGGER_INFO(TAG,"%s Log service started (PID: %d)\n",PREFIX, getpid());

    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    hd_service_interface interface =  {
        .hd_service_init = on_init,
        .hd_service_on_start = on_start,
        .hd_service_on_destory = on_destory
    };

    /* 接受父进程的socked fd 进行通信  */
    int sock_fd = atoi(argv[1]);  // 获取父进程传递的 socket fd

    char buffer[128];
    // 返回给父进程表明启动成功 : <进程名称>,<进程id>,<程序版本号> 
    // sprintf(buffer,"%s,%d,%s","hdlog",getpid(),VERSION);
    const int  sid = getpid();

    hd_child_info_encode(buffer,"hdlog",sid,VERSION);

    write(sock_fd, buffer, strlen(buffer));

    close(sock_fd);
    /* 接受父进程的socked fd 进行通信  end*/

    

    //signal(SIGKILL, handle_signal);
    int index = 0;
    // 日志服务逻辑

    //int ret = kill(getppid(), SIGUSR1);  // 向父进程发送信号

    /*
    sleep(1);
    // if (ret !=0)
    // {
    //     HD_LOGGER_ERROR(TAG,">>>>>> kill %d(%s)\n",errno,strerror(errno));
    // }else{
        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un addr = { .sun_family = AF_UNIX };
        strcpy(addr.sun_path, HD_IPC_SOCKET_PATH_FOR_CHILD);
        connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
        char buffer[128];
        sprintf(buffer,"%s,%d","hdlog",getpid());
        while (!confirm)
        {
            sleep(1);
            //HD_LOGGER_INFO(TAG,">>>>>> NOTIFY STARTED <<<<<<<\n");
            write(sockfd, buffer, strlen(buffer));
        }
        close(sockfd);
    // }
    */
    

    while (running) {
        time_t now = time(NULL);
        HD_LOGGER_INFO(TAG,"%s Log service heartbeat:%d\n",PREFIX, index++);
        sleep(15);
    }
    
    HD_LOGGER_INFO(TAG,"%s Log service stopped !!!\n",PREFIX);

    return 0;
}