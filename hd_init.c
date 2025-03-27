#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include "hd_init.h"
#include "hd_logger.h"


#define TAG                         "hdinit"
#define DEBUG                       0
#define DEBUG_LOCAL_PATH            1
#define MONITOR_SERVICES_INTERVAL   5 
#define MAX_SERVICES                20
#define ERROR_CHILD_START_FAIL      99

#define HD_INIT_SERVICE_MAIN        "hdmain"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_MAIN_PATH   "./hdmain"
#else
#define HD_INIT_SERVICE_MAIN_PATH   "/root/hdmain"
#endif

#define HD_INIT_SERVICE_LOG         "hdlog"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_LOG_PATH    "./hdlog"
#else
#define HD_INIT_SERVICE_LOG_PATH    "/root/hdlog"
#endif

#define HD_INIT_SERVICE_SHELL       "hdshell"
#if DEBUG_LOCAL_PATH
#define HD_INIT_SERVICE_SHELL_PATH  "./hdshell"
#else 
#define HD_INIT_SERVICE_SHELL_PATH  "/root/hdshell"
#endif

#if DEBUG
#define DEBUG_THREAD
#else 
#endif

Service services[MAX_SERVICES];
int service_count = 0;
atomic_int service_manager_running = 1;

time_t get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}


typedef struct {
    pid_t pid;
    int service_index;
} Wait_Child_Exit_Thread_Args;

void * wait_child_exit_thread(void * arg){
    
    Wait_Child_Exit_Thread_Args *args = (Wait_Child_Exit_Thread_Args*)arg;
    int pid = args->pid;
    int index = args->service_index;
    free(args);
    HD_LOGGER_ERROR(TAG,"wait_child_exit_thread[%d %s %s] Parent:[%d-%d] wait exit ... \n",index,services[index].name,services[index].path,pid,getpid());
    int status;
    if (!service_manager_running)return NULL;
    waitpid(pid,&status,0);// 等待子进程结束
    if (!service_manager_running)return NULL;
    if (WIFEXITED(status)){
        HD_LOGGER_ERROR(TAG,"wait_child_exit_thread[%d %s %s] Parent:[%d-%d]! child exited with status %d\n",index,services[index].name,services[index].path,pid,getpid(),WEXITSTATUS(status));
        if(WEXITSTATUS(status) == ERROR_CHILD_START_FAIL){
            HD_LOGGER_ERROR(TAG,"wait_child_exit_thread[%d %s %s] Parent:[%d-%d]! child execl() failed in child!\n",index,services[index].name,services[index].path,pid,getpid());  
        }
        // 更新Service
        services[index].is_running = 0;
    }
    return NULL;
}

int service_find_index_by_name(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int service_start_by_index(int index) {
    for (int i = 0; i < services[index].depends_on_count; i++) {
        int found = 0;
        for (int j = 0; j < service_count; j++) {
            if (strcmp(services[j].name, services[index].depends_on[i]) == 0) {
                found = 1;
                if (!services[j].is_running) {
                    return -1;
                }
                break;
            }
        }
        if (!found) {
            return -1;
        }
    }
    HD_LOGGER_INFO(TAG,"    service_start_by_index[%d %s %s] fork ...\n",index,services[index].name,services[index].path);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        HD_LOGGER_ERROR(TAG,"    service_start_by_index[%d %s %s] fork failed!\n",index,services[index].name,services[index].path);
        return -1;
    } else if (pid == 0) {
        // 子进程分支
        HD_LOGGER_INFO(TAG,"    service_start_by_index[%d %s %s] fork success! Child:[%d-%d] do execl. \n",index,services[index].name,services[index].path,getppid(),getpid());
        int ret = execl(services[index].path, services[index].name, NULL); // 成功时无返回值，失败返回 -1
        HD_LOGGER_ERROR(TAG,"    service_start_by_index[%d %s %s] Child execl fail : %d! \n",index,services[index].name,services[index].path,ret);
        // exit(EXIT_FAILURE);
        _exit(ERROR_CHILD_START_FAIL); // 通知Parent
    } else {
        // 父进程分支
        HD_LOGGER_INFO(TAG,"    service_start_by_index[%d %s %s] fork success！Parent:[%d-%d]! \n",index,services[index].name,services[index].path,pid,getpid());
        services[index].pid = pid;
        services[index].is_running = 1;
        services[index].last_modified = get_last_modified(services[index].path);

        // 开启线程监听子进程结束 TODO 处理线程生命周期
        pthread_t wait_child_exit_thread_t;
        // -动态分配结构体（确保线程访问时内存有效）
        Wait_Child_Exit_Thread_Args *args = malloc(sizeof(Wait_Child_Exit_Thread_Args));
        args->pid = pid;
        args->service_index = index;
        pthread_create(&wait_child_exit_thread_t,NULL,wait_child_exit_thread,args);
        return 0;
    }
}

int init_core_services(){
    // todo 检查path

    // 主服务
    HD_LOGGER_INFO(TAG,"    init_core_services: %s %s \n",HD_INIT_SERVICE_MAIN,HD_INIT_SERVICE_MAIN_PATH);
    strcpy(services[0].name, HD_INIT_SERVICE_MAIN);
    strcpy(services[0].path, HD_INIT_SERVICE_MAIN_PATH);
    services[0].is_main = 1;
    services[0].is_secondary = 0;
    services[0].is_registered = 1;
    services[0].depends_on_count = 0;
    strcpy(services[0].depends_on[0], HD_INIT_SERVICE_LOG);
    
    // 日志服务
    HD_LOGGER_INFO(TAG,"    init_core_services: %s %s \n",HD_INIT_SERVICE_LOG,HD_INIT_SERVICE_LOG_PATH);
    strcpy(services[1].name, HD_INIT_SERVICE_LOG);
    strcpy(services[1].path, HD_INIT_SERVICE_LOG_PATH);
    services[1].is_main = 0;
    services[1].is_secondary = 1;
    services[1].is_registered = 1;
    services[1].depends_on_count = 0;
    strcpy(services[1].depends_on[0], HD_INIT_SERVICE_LOG);
    
     // shell服务
    HD_LOGGER_INFO(TAG,"    init_core_services: %s %s \n",HD_INIT_SERVICE_SHELL,HD_INIT_SERVICE_SHELL_PATH);
    strcpy(services[2].name, HD_INIT_SERVICE_SHELL);
    strcpy(services[2].path, HD_INIT_SERVICE_SHELL_PATH);
    services[2].is_main = 0;
    services[2].is_secondary = 1;
    services[2].is_registered = 1;
    services[2].depends_on_count = 1;
    strcpy(services[2].depends_on[0], HD_INIT_SERVICE_LOG);

    service_count = 3;
    HD_LOGGER_INFO(TAG,"    init_core_services service_count: %d  \n",service_count);
    return 0;

}

void * ipc_server_thread(void *arg){

    return NULL;
}

void * init_debug_thread(void *arg){
    sleep(10);
    service_manager_running = 0;
    return NULL;
} 

int start_core_services(){
    int index ;
    int ret ;
    
    // 启动日志服务
    HD_LOGGER_INFO(TAG,"    start service : %s %s\n",HD_INIT_SERVICE_LOG,HD_INIT_SERVICE_LOG_PATH);
    index = service_find_index_by_name(HD_INIT_SERVICE_LOG);
    if (index<0){
        HD_LOGGER_ERROR(TAG,"can not find : %s %s fail!\n",HD_INIT_SERVICE_LOG,HD_INIT_SERVICE_LOG_PATH);
        return -1;
    }
    ret = service_start_by_index(index);
    if (ret<0)
    {
        HD_LOGGER_ERROR(TAG,"can not start : %s %s fail\n",HD_INIT_SERVICE_LOG,HD_INIT_SERVICE_LOG_PATH);
        return -1;
    }
    
    // 启动主服务
    HD_LOGGER_INFO(TAG,"    start service : %s %s\n",HD_INIT_SERVICE_MAIN,HD_INIT_SERVICE_MAIN_PATH);
    index = service_find_index_by_name(HD_INIT_SERVICE_MAIN);
    if (index<0){
        HD_LOGGER_ERROR(TAG,"can not find : %s %s fail!\n",HD_INIT_SERVICE_MAIN,HD_INIT_SERVICE_MAIN_PATH);
        return -1;
    }
    ret = service_start_by_index(index);
    if (ret<0)
    {
        HD_LOGGER_ERROR(TAG,"can not start : %s %s fail\n",HD_INIT_SERVICE_LOG,HD_INIT_SERVICE_LOG_PATH);
        return -1;
    }

    // 启动shell
    HD_LOGGER_INFO(TAG,"    start service : %s %s\n",HD_INIT_SERVICE_SHELL,HD_INIT_SERVICE_SHELL_PATH);
    index = service_find_index_by_name(HD_INIT_SERVICE_SHELL);
    if (index<0){
        HD_LOGGER_ERROR(TAG,"can not find : %s %s fail!\n",HD_INIT_SERVICE_SHELL,HD_INIT_SERVICE_SHELL_PATH);
        return -1;
    }
    ret = service_start_by_index(index);
    if (ret<0)
    {
        HD_LOGGER_ERROR(TAG,"can not start : %s %s fail\n",HD_INIT_SERVICE_SHELL,HD_INIT_SERVICE_SHELL_PATH);
        return -1;
    }
    HD_LOGGER_INFO(TAG,"ipc_thread ... \n");
    // 启动IPC服务器线程 TODO 处理线程
    pthread_t ipc_thread; // 
    if (pthread_create(&ipc_thread, NULL, ipc_server_thread, NULL) != 0) {
        perror("pthread_create ipc_thread failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void monitor_services(){

}

int main(int argc, char const *argv[])
{
    hd_logger_set_level(HD_LOGGER_LEVEL_INFO);
    HD_LOGGER_INFO(TAG,"================================================\n");
    HD_LOGGER_INFO(TAG,"Starting hd_init service manager (version:%d ) ...\n",VERSION);
    HD_LOGGER_INFO(TAG,"================================================\n");

    int ret;
    HD_LOGGER_INFO(TAG,"init_core_services ... \n");
    ret = init_core_services();
    if (ret<0){
        HD_LOGGER_ERROR(TAG,"init_core_services error! \n");
        exit(EXIT_FAILURE); 
    }
#ifdef DEBUG_THREAD
    HD_LOGGER_INFO(TAG,"debug_thread ... \n");
        // 启动测试线程
    pthread_t debug_thread;
    if (pthread_create(&debug_thread, NULL, init_debug_thread, NULL) != 0) {
        perror("pthread_create debug_thread failed");
        exit(EXIT_FAILURE);
    }    
#endif  // DEBUG_THREAD
    
    HD_LOGGER_INFO(TAG,"start_core_services ... \n");
    ret = start_core_services();
    if (ret<0){
        HD_LOGGER_ERROR(TAG,"start_core_services error! \n");
        exit(EXIT_FAILURE); 
    }
    
    while (service_manager_running) {
        HD_LOGGER_INFO(TAG,"monitor_services ... \n");
        monitor_services();
        sleep(MONITOR_SERVICES_INTERVAL);
    }

    HD_LOGGER_INFO(TAG,"end! \n");
    return 0;
}





