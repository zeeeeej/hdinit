
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "hd_service_interface.h"
#include "hd_ipc.h"
#include "hd_utils.h"
#include "hd_logger.h"
#include <execinfo.h>

#define TIMEOUT_SEC 10.0

int hd_service_interface_running = 1; // 1:stop ; 0:start.

static int sleep_internal = 10;
static char *g_service_name = NULL ;

static hd_service_interface_on_parent_exit_child my_hd_on_parent_exit_child = NULL;

// 线程控制标志
static volatile int timeout_thread_running = 0;
static pthread_t timeout_thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int heartbeat_received = 0;


// 超时处理函数
static void *heart_beat_timeout_handler(void *arg)
{
    time_t last_heartbeat = time(NULL);
    while (1) {
        pthread_mutex_lock(&mutex);
        if (heartbeat_received) {
            last_heartbeat = time(NULL);
            heartbeat_received = 0;
            printf("收到心跳，重置计时器\n");
        }
        pthread_mutex_unlock(&mutex);
        double diff = difftime(time(NULL), last_heartbeat);
        if (diff >= TIMEOUT_SEC) {
            printf("超时！%f秒内未收到心跳信号 diff=%f\n", TIMEOUT_SEC,diff);
            last_heartbeat = time(NULL);  // 重置计时器
            break;
        }
        
        sleep(1);  // 每秒检查一次
    }
    hd_service_interface_running = 1;
    return NULL;
}

static void segv_handler(int sig)
{
    void *array[10];
    size_t size = backtrace(array, 10); // 获取调用栈
    fprintf(stderr, "Segmentation fault! Backtrace:\n");
    backtrace_symbols_fd(array, size, STDERR_FILENO); // 打印调用栈
    exit(1);
}

static void handle_signal(int sig)
{
    HD_LOGGER_INFO("hd_service_interface", "handle_signal sig=%d\n", sig);
    if (sig == SIGUSR1)
    {
        HD_LOGGER_INFO("hd_service_interface", "SIGUSR1\n");
    }
    else if (sig == SIGUSR2 || sig == SIGTERM)
    {
        HD_LOGGER_INFO("hd_service_interface", "1111\n");
        hd_service_interface_running = 1;
        HD_LOGGER_INFO("hd_service_interface", "22222\n");
        if (my_hd_on_parent_exit_child != NULL)
        {
            HD_LOGGER_INFO("hd_service_interface", "my_hd_on_parent_exit_child invoke! \n", sig);
            //*g_running = 0; // error
            my_hd_on_parent_exit_child();
        }
        else
        {
            HD_LOGGER_WARNING("hd_service_interface", "my_hd_on_parent_exit_child is NULL! \n", sig);
        }
    }
    else
    {
        HD_LOGGER_INFO("hd_service_interface", "OTHER\n");
    }
    HD_LOGGER_INFO("hd_service_interface", "handle_signal sig=%d    end.\n", sig);
}

static void sig_heart_beat_handler(int sig, siginfo_t *info, void *ucontext)
{
    // printf(" \n");
    // printf(" -----  SIG            :               %d\n", sig);
    // printf(" -----  Pid            :               %d\n", info->si_pid);
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<< PONG [%s] %ds \n",g_service_name,sleep_internal);
    heartbeat_received = 1;

    sleep(sleep_internal);
 
    kill(info->si_pid, sig);

}

static void hd_service_interface_reply_to_parent(

    const char *socket_fd,
    const char *service_name,
    const char *version)
{
    /* 接受父进程的socked fd 进行通信  */
    int sock_fd = atoi(socket_fd); // 获取父进程传递的 socket fd
    if (sock_fd < 0)
    {
        printf("sock_fd=[%d] fail! \n", sock_fd);
        return;
    }

    char buffer[HD_IPC_SOCKET_PATH_FOR_CHILD_BUFF_SIZE];
    // 返回给父进程表明启动成功 : <进程名称>,<进程id>,<程序版本号>
    // sprintf(buffer,"%s,%d,%s","hdlog",getpid(),VERSION);
    const int sid = getpid();

    hd_child_info_encode(buffer, service_name, sid, version);

    write(sock_fd, buffer, strlen(buffer));

    close(sock_fd);

    /* 接受父进程的socked fd 进行通信  end*/

    hd_service_interface_running = 0;
}

static void hd_service_interface_recv_from_parent(hd_service_interface_on_parent_exit_child callback)
{
    my_hd_on_parent_exit_child = callback;

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    signal(SIGSEGV, segv_handler);
}

static void hd_service_interface_heartbeat()
{
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sig_heart_beat_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sigaddset(&sa_usr1.sa_mask, SIGTERM); // 阻塞 SIGTERM
    sa_usr1.sa_flags = 0;

    sigaction(SIGUSR1, &sa_usr1, NULL);
}

int hd_service_interface_init(
    const char *socket_fd,
    const char *service_name,
    const char *version,
    int heart_beat,
    hd_service_interface_on_parent_exit_child callback)
{
    sleep_internal = heart_beat;
    // strncpy(g_service_name,service_name,sizeof(g_service_name)-1);
    // g_service_name[sizeof(g_service_name)-1] = '0';
    // 动态分配新内存
    g_service_name = strdup(service_name);  // 或 malloc + strcpy

    pthread_create(&timeout_thread, NULL, heart_beat_timeout_handler, NULL);
 
    hd_service_interface_reply_to_parent(socket_fd, service_name, version);
    hd_service_interface_recv_from_parent(callback);
    hd_service_interface_heartbeat();
    return 0;
}

void hd_service_interface_destory()
{ 
    if (g_service_name) {
        free(g_service_name);
    }
    hd_service_interface_running = 1;
}