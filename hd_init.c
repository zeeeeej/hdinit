#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include "hd_init.h"
#include <signal.h>
#include <sys/reboot.h>
#include "hd_logger.h"
#include "hd_utils.h"



#define SPIT ""
#define TAG "hdinit"
#define DEBUG 0
#define DEBUG_LOCAL_PATH 1
#define MONITOR_SERVICES_INTERVAL 5
#define MAX_SERVICES 20
#define MAX_RESTART_MAIN 3
#define ERROR_CHILD_START_FAIL 99

#define HD_INIT_SERVICE_MAIN "hdmain"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_MAIN_PATH "./hdmain"
#else
#define HD_INIT_SERVICE_MAIN_PATH "/root/hdmain"
#endif

#define HD_INIT_SERVICE_LOG "hdlog"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_LOG_PATH "./hdlog"
#else
#define HD_INIT_SERVICE_LOG_PATH "/root/hdlog"
#endif

#define HD_INIT_SERVICE_SHELL "hdshell"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_SHELL_PATH "./hdshell"
#else
#define HD_INIT_SERVICE_SHELL_PATH "/root/hdshell"
#endif

#if DEBUG
#define DEBUG_THREAD
#else
#endif

/**
 * 所有服务
 */
HDServiceArray g_service_array;

/**
 * 服务管理是否运行
 */
atomic_int service_manager_running = 1;

/**
 * 连续启动主服务次数
 */
atomic_int start_main_service_count = 0;

atomic_int service_manager_running_count = 0;


typedef struct
{
     HDService * service;
    // const char *service_name;
} Wait_Child_Exit_Thread_Args;

/**
 * 主进程等待子进程退出 
 * 线程函数
 */
void *wait_child_exit_thread(void *arg)
{
    //hd_service_array_print(&g_service_array);
    Wait_Child_Exit_Thread_Args *args = (Wait_Child_Exit_Thread_Args *)arg;
    pthread_t current_tid = pthread_self();
    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread arg=%s p:%p [%lu]\n",SPIT, args->service->name, args->service,(unsigned long)current_tid);
    hd_service_array_print(&g_service_array);
     HDService *service = args->service;//hd_service_array_find_by_name(&g_service_array, args->service_name);
    if (service==NULL)
    {
       free(args);
       return NULL;
    }
    
    char *s_name = service->name;
    char *s_path = service->path;
    pid_t pid = service->pid;
    free(args);

    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] wait exit ... \n",SPIT, s_name, s_path, getpid(), pid);
    
    int status;
   
    waitpid(pid, &status, 0); // 等待子进程结束

    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d]exited with status <%d|%d>\n",SPIT, s_name, s_path, getpid(), pid, status, WEXITSTATUS(status));
    
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == ERROR_CHILD_START_FAIL )
        {
            HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] child execl() failed! %d \n",SPIT, s_name, s_path, getpid(),pid,start_main_service_count);
            if (strcmp(s_name ,HD_INIT_SERVICE_MAIN)==0)
            {
                HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] hdmain execl() failed!start_main_service_count++ %d \n", SPIT,s_name, s_path, getpid(),pid,start_main_service_count);
                start_main_service_count++;
            }
        }
    }

    // 更新service
    service->is_running = 0;
  
    return NULL;
}



/**
 * 启动服务
 */
static int op_start_service_internal( HDService *service)
{
    
    if (service == NULL)
    {
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s] can not find!\n", service->name, service->path);
        return -1;
    }
    char *start_name = service->name;
    char *start_path = service->path;
    HD_LOGGER_INFO(TAG, "op_start_service_internal----------------\n");
    HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] p->%p fork ...\n", start_name, start_path,service);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s] fork failed!\n", start_name, start_path);
        return -1;
    }
    else if (pid == 0)
    {
        // 子进程分支
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s]Child:[%d/%d] do execl... \n", start_name, start_path,  getppid(),getpid());
        int ret = execl(service->path, service->name, NULL); // 成功时无返回值，失败返回 -1
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s]Child but Child execl() fail : %d! \n", start_name, start_path, ret);
        // exit(EXIT_FAILURE);
        _exit(ERROR_CHILD_START_FAIL); // 通知Parent
    }
    else
    {
        // 父进程分支
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] Parent:[%d/%d]! \n", start_name, start_path, getpid(), pid);
        service->is_running = 1;
        service->pid = pid;
        service->last_modified = get_last_modified(start_path);

        // 开启线程监听子进程结束 TODO 处理线程生命周期
        pthread_t wait_child_exit_thread_t;
        // -动态分配结构体（确保线程访问时内存有效）
        Wait_Child_Exit_Thread_Args *args = malloc(sizeof(Wait_Child_Exit_Thread_Args));
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] Parent arg:%s p:%p\n", start_name, start_path,service->name,service);
        //args->service_name = service->name;
        args->service = service;
        pthread_create(&wait_child_exit_thread_t, NULL, wait_child_exit_thread, args);
        return 0;
    }
}

/**
 * 根据服务名称启动服务
 */
static int op_start_service_by_name(const char *service_name){
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    return op_start_service_internal(service);
}

/**
 * 初始化核心服务
 */
static int init_core_services()
{
    // todo 检查path

    // 主服务
    HD_LOGGER_INFO(TAG, "init_core_services[%s:%s] ...\n", HD_INIT_SERVICE_MAIN, HD_INIT_SERVICE_MAIN_PATH);
    HDService main_service = {
        .name = HD_INIT_SERVICE_MAIN,
        .path = HD_INIT_SERVICE_MAIN_PATH,
        .is_main = 1,
        .is_secondary = 0,
        .is_registered = 1,
        .depends_on_count = 1,
        .version = "0.0.0",
        .depends_on = {HD_INIT_SERVICE_LOG}};
    hd_service_array_add(&g_service_array, &main_service);

    // 日志服务
    HD_LOGGER_INFO(TAG, "init_core_services[%s:%s] ...\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
    HDService log_service = {
        .name = HD_INIT_SERVICE_LOG,
        .path = HD_INIT_SERVICE_LOG_PATH,
        .is_main = 0,
        .is_secondary = 1,
        .is_registered = 1,
        .depends_on_count = 1,
        .version = "0.0.0",
        .depends_on = {}};
    hd_service_array_add(&g_service_array, &log_service);

    // // shell服务
    // HD_LOGGER_INFO(TAG, "init_core_services[%s:%s] ...\n", HD_INIT_SERVICE_SHELL, HD_INIT_SERVICE_SHELL_PATH);
    // HDService shell_service = {
    //     .name = HD_INIT_SERVICE_SHELL,
    //     .path = HD_INIT_SERVICE_SHELL_PATH,
    //     .is_main = 0,
    //     .is_secondary = 1,
    //     .is_registered = 1,
    //     .depends_on_count = 1,
    //     .depends_on = {HD_INIT_SERVICE_LOG}};
    // hd_service_array_add(&g_service_array, &shell_service);

    return 0;
}

/**
 * 处理shell命令
 * 线程函数
 */
void *ipc_server_thread(void *arg)
{

    return NULL;
}

/**
 * DEBUG
 * 线程函数
 */
void *init_debug_thread(void *arg)
{
    sleep(10);
    service_manager_running = 0;
    return NULL;
}

/**
 * 启动核心服务
 */
static int start_core_services()
{
    int ret;

    

    // 启动主服务
    HD_LOGGER_INFO(TAG, "start service[%s:%s] ...", HD_INIT_SERVICE_MAIN, HD_INIT_SERVICE_MAIN_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_MAIN);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start service[%s:%s]start fail!\n", HD_INIT_SERVICE_MAIN, HD_INIT_SERVICE_MAIN_PATH);
        return -1;
    }

    // 启动日志服务
    HD_LOGGER_INFO(TAG, "start service[%s:%s] ...\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_LOG);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start service[%s:%s]start fail!\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
        return -1;
    }

    // // 启动shell
    // HD_LOGGER_INFO(TAG, "start service[%s:%s] ...\n", HD_INIT_SERVICE_SHELL, HD_INIT_SERVICE_SHELL_PATH);
    // ret = op_start_service_by_name(HD_INIT_SERVICE_SHELL);
    // if (ret < 0)
    // {
    //     HD_LOGGER_ERROR(TAG, "start service[%s:%s]start fail!\n", HD_INIT_SERVICE_SHELL, HD_INIT_SERVICE_SHELL_PATH);
    //     return -1;
    // }

    // 启动IPC服务器线程 TODO 处理线程
    HD_LOGGER_INFO(TAG, "ipc_thread ... \n");
    pthread_t ipc_thread; //
    if (pthread_create(&ipc_thread, NULL, ipc_server_thread, NULL) != 0)
    {
        perror("pthread_create ipc_thread failed!");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static int op_check_service_update_internal(HDService *service)
{

    return 0;
}

static int op_update_service_internal(HDService *service)
{

    return 0;
}

/**
 * 停止服务
 */
static int op_stop_service_internal(const HDService *service)
{
    if (service == NULL)
    {
        return -1;
    }
    const char *s_name = service->name;
    const char *s_path = service->path;
    pid_t child_pid = service->pid;
    HD_LOGGER_ERROR(TAG, "op_stop_service_internal : %s %s ......\n", s_name, s_path);
    if (service->is_running)
    {
        if (kill(child_pid, SIGTERM) == 0)
        {
            sleep(1); // 给子进程时间清理
            int status;
            // 检查子进程是否还在运行
            if (waitpid(child_pid, &status, WNOHANG) == 0)
            {
                // 如果还在运行，强制终止
                kill(child_pid, SIGKILL);
                // waitpid(child_pid, &status, 0);
                // service->is_running = 0;
                return 0;
            }
            else
            {
                perror("kill failed");
                return -1;
            }
        }
        return 0;
    }
    return 0;
}

/**
 * 根据服务名称停止服务
 */
static int do_stop_service_by_name(const char *service_name){
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    return op_stop_service_internal(service);
}


/**
 * 停止所有服务
 * 重启
 */
static void opt_reboot_internal(){
    HD_LOGGER_DEBUG(TAG,"opt_reboot_internal !!! \n");
    sync();  // 确保数据写入磁盘
    sleep(1);
    for(int i=0;i<g_service_array.count;i++){
        op_stop_service_internal(&g_service_array.services[i]);
    }
    service_manager_running = 0;

    //reboot(RB_AUTOBOOT);  // 发起重启
}

/**
 * 处理重启信号
 */
void signal_handle_reboot(int sig) {
    HD_LOGGER_INFO("hdlog",">>>>> signal_handle_reboot <<<<<< \n");
    opt_reboot_internal();
}

/**
 * 检测服务状态
 */
static void monitor_services(){


    if (start_main_service_count>MAX_RESTART_MAIN)
    {
        // todo 发出重启信号
        opt_reboot_internal();
        return;
    }
    

    for (int i = 0; i < g_service_array.count; i++)
    {
        HDService *service = g_service_array.services+i; 
        
        // if (!service->is_running)
        // {
            HD_LOGGER_DEBUG(TAG, "%-8s#monitor_services# [%s] %s ... \n",SPIT,service->name,HD_SERVICE_RUNNING_TEXT(service->is_running));
            if (kill(service->pid, 0) != 0)
            {
                service->is_running = 0;  // 标记服务为"未运行"
                op_start_service_internal(service); // 重新启动服务
            }else{
                if (strcmp(service->name ,HD_INIT_SERVICE_MAIN)==0)
                {
                    start_main_service_count=0;
                }
            }
        // }else{

        // }
    }
}

/**
 * 检测服务升级
 */
static void upgrade_services(){
    for (int i = 0; i < g_service_array.count; i++)
    {
        HDService *service = g_service_array.services+i;    
        HD_LOGGER_DEBUG(TAG, "%-8s#upgrade_services# [%s] version:%s ... \n",SPIT,service->name,service->version);
        if (op_check_service_update_internal(service)!=0)
        {
            op_stop_service_internal(service);
            op_update_service_internal(service);
            op_start_service_internal(service);
        }
    }
}

void * monitor_thread_services_thread(void * arg){
    /* 开始循环 检测核心服务是否运行 */
    while (service_manager_running)
    {
        sleep(MONITOR_SERVICES_INTERVAL);
        HD_LOGGER_INFO(TAG, "#monitor_services# %d-%d ... \n",start_main_service_count,service_manager_running);
        monitor_services();
        service_manager_running_count++;
    }
    return NULL;
}

void * upgrade_thread_services_thread(void * arg){
    /* 开始循环 检测核心服务是否需要升级 */
    while (service_manager_running)
    {
        sleep(MONITOR_SERVICES_INTERVAL);
        HD_LOGGER_INFO(TAG, "#upgrade_services# ... \n");
        upgrade_services();
    }
    return NULL;
}


/**
 *  gcc hd_init.c hd_logger.c hd_service.c -o hd_init -lpthread
 */
int main(int argc, char const *argv[])
{
    hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    HD_LOGGER_INFO(TAG, "================================================\n");
    HD_LOGGER_INFO(TAG, "Starting hd_init service manager (version:%d ) ...\n", VERSION);
    HD_LOGGER_INFO(TAG, "================================================\n");
    int ret;

    /* 初始化core服务 */
    HD_LOGGER_INFO(TAG, "init_core_services ... \n");
    hd_service_array_init(&g_service_array);
    ret = init_core_services();
    hd_service_array_print(&g_service_array);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "init_core_services error! \n");
        exit(EXIT_FAILURE);
    }

    /* DEBUG */
#ifdef DEBUG_THREAD
    HD_LOGGER_INFO(TAG, "debug_thread ... \n");
    // 启动测试线程
    pthread_t debug_thread;
    if (pthread_create(&debug_thread, NULL, init_debug_thread, NULL) != 0)
    {
        perror("pthread_create debug_thread failed");
        exit(EXIT_FAILURE);
    }
#endif // DEBUG_THREAD

    /* 启动core服务 */
    HD_LOGGER_INFO(TAG, "start_core_services ... \n");
    ret = start_core_services();
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start_core_services error! \n");
        exit(EXIT_FAILURE);
    }




    HD_LOGGER_INFO(TAG, "monitor_services_thread_t started !!! \n");
    pthread_t monitor_services_thread_t;
    pthread_create(&monitor_services_thread_t,NULL,monitor_thread_services_thread,NULL);
    HD_LOGGER_INFO(TAG, "upgrade_services_thread_t started !!! \n");
    pthread_t upgrade_services_thread_t;
    pthread_create(&upgrade_services_thread_t,NULL,upgrade_thread_services_thread,NULL);

    pthread_join(monitor_services_thread_t,NULL);
    HD_LOGGER_INFO(TAG, "monitor_services_thread_t end !!! \n");
    pthread_join(upgrade_services_thread_t,NULL);
    HD_LOGGER_INFO(TAG, "upgrade_services_thread_t end !!! \n");

    HD_LOGGER_INFO(TAG, "end! \n");

    //printf("准备自杀...\n");
    //kill(getpid(), SIGKILL);  // SIGTERM 更温和
    //kill(getpid(), SIGTERM);  // SIGTERM 更温和
    //printf("这行不会执行\n");
    hd_service_array_cleanup(&g_service_array);
    return 0;
}
