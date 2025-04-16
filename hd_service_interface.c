
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

#define TIMEOUT_SEC 10

int hd_service_interface_running = 1; // 1:stop ; 0:start.
static char *g_service_name = NULL ;
static hd_service_interface_on_parent_exit_child my_hd_on_parent_exit_child = NULL;
static volatile int timeout_thread_running = 0;
static pthread_t timeout_thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int heartbeat_received = 0;

// 超时处理函数
static void *heart_beat_timeout_handler(void *arg)
{
 	printf("heart_beat_timeout_handler ...\n");
    time_t last_heartbeat = time(NULL);
    while (1) {

    	printf("checking ... ... %d %ld \n",heartbeat_received,last_heartbeat);
 /*       pthread_mutex_lock(&mutex);
        if (heartbeat_received) {
            last_heartbeat = time(NULL);
            heartbeat_received = 0;
            printf("收到心跳，重置计时器\n");
        }
        pthread_mutex_unlock(&mutex);
        
        if (difftime(time(NULL), last_heartbeat) >= TIMEOUT_SEC) {
            printf("超时！%d秒内未收到心跳信号\n", TIMEOUT_SEC);
            last_heartbeat = time(NULL);  // 重置计时器
   	    hd_service_interface_running = 1;
            break;
        }
     */   
        sleep(1);  // 每秒检查一次
    }
    timeout_thread_running = 0;
    return NULL;
}

// 启动超时计时器
static void start_timeout()
{
    if (timeout_thread_running==0)
    {
	    
    	printf("timeout thread creatintg ...\n");
        timeout_thread_running = 1;
        pthread_create(&timeout_thread, NULL, heart_beat_timeout_handler, NULL);
    }else{
    	printf("timeout thread is already create!\n");
	    //timeout_thread_running = 1;
    }
}

// // 取消超时计时器
// static void cancel_timeout()
// {
//     if (timeout_thread_running)
//     {
//         timeout_thread_running = 0;
//         pthread_cancel(timeout_thread);
//         pthread_join(timeout_thread, NULL); // 等待线程结束
//     }
// }

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
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<< PONG [%s]\n",g_service_name);
    
    // reset
    pthread_mutex_lock(&mutex); 
    heartbeat_received = 1;
    pthread_mutex_unlock(&mutex);
    
    sleep(5);
    
    kill(info->si_pid, sig);
    
    //start_timeout();
    
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

    	HD_LOGGER_ERROR("hd_service_interface","===>1\n");
    hd_child_info_encode(buffer, service_name, sid, version);

    	HD_LOGGER_ERROR("hd_service_interface","====>2\n");
   ssize_t size  =  write(sock_fd, buffer, strlen(buffer));
	
    	HD_LOGGER_ERROR("hd_service_interface","====>3%zd \n",size);
    // close(sock_fd);

    	HD_LOGGER_ERROR("hd_service_interface","====>4\n");
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

    //signal(SIGSEGV, segv_handler);
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

void handle_sigbus(int sig){

    	HD_LOGGER_ERROR("hd_service_interface","SIGBUS:MEMORY access error!");
	exit(EXIT_FAILURE);
}

int hd_service_interface_init(
    const char *socket_fd,
    const char *service_name,
    const char *version,
    int heart_beat,
    hd_service_interface_on_parent_exit_child callback)
{
	
    signal(SIGPIPE,SIG_IGN);
    signal(SIGBUS,handle_sigbus);
    //sleep_internal = heart_beat;
    // strncpy(g_service_name,service_name,sizeof(g_service_name)-1);
    // g_service_name[sizeof(g_service_name)-1] = '0';

    // 动态分配新内存
    g_service_name = strdup(service_name);  // 或 malloc + strcpy

    if(!g_service_name){
    	HD_LOGGER_ERROR("hd_service_interface","strdup error!");
	exit(EXIT_FAILURE);
    }

    	HD_LOGGER_ERROR("hd_service_interface","11111111111111111111111\n");
    pthread_create(&timeout_thread, NULL, heart_beat_timeout_handler, NULL);
    
    	HD_LOGGER_ERROR("hd_service_interface","22222222222222222222222222\n");
    hd_service_interface_reply_to_parent(socket_fd, service_name, version);
    	HD_LOGGER_ERROR("hd_service_interface","33333333333333333333333333\n");
   // hd_service_interface_recv_from_parent(callback);
   // hd_service_interface_heartbeat();
   
    return 0;
}

void hd_service_interface_destory()
{

  // 通知线程退出
    hd_service_interface_running = 1;
    
    // 等待线程结束
    if (timeout_thread) {
        pthread_join(timeout_thread, NULL);
        timeout_thread = 0;
    }
    
    // 释放资源
    if (g_service_name) {
        free(g_service_name);
        g_service_name = NULL;
    }

    	HD_LOGGER_ERROR("hd_service_interface","hd_service_interface_destory");
}
