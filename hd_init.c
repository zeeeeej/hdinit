#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include "hd_init.h"
#include <signal.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "hd_logger.h"
#include "hd_utils.h"
#include "hd_ipc.h"

#define PREFIX "######"

static void opt_reboot_internal();
static int op_stop_service_internal( HDService *service);
static int op_check_service_update_internal(HDService *service);
static void hd_init_exit();

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

    if (!service_manager_running)
    {
        return NULL;
    }
    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d]exited with status <%d|%d>\n",SPIT, s_name, s_path, getpid(), pid, status, WEXITSTATUS(status));
    
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == ERROR_CHILD_START_FAIL )
        {
            HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] child execl() failed! %d \n",SPIT, s_name, s_path, getpid(),pid,start_main_service_count);
            if (strcmp(s_name ,HD_INIT_SERVICE_MAIN)==0)
            {
                start_main_service_count++;
                HD_LOGGER_ERROR(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] hdmain execl() failed!start_main_service_count %d \n", SPIT,s_name, s_path, getpid(),pid,start_main_service_count);
            }
        }
    }

    // 更新service
    service->status = HD_SERVICE_STATUS_STOPPED;

    
  
    return NULL;
}



/**
 * 启动服务
 */
static int op_start_service_internal( HDService *service)
{
    if (!service_manager_running)
    {
        return -1;
    }
    
    if (service == NULL)
    {
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s] can not find!\n", service->name, service->path);
        return -1;
    }
    char *start_name = service->name;
    char *start_path = service->path;
    HD_LOGGER_INFO(TAG, "op_start_service_internal----------------> \n");
    HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] p->%p fork ...\n", start_name, start_path,service);

    // 开启socket，子进程通过这个socket传递自己的数据，比如已启动。
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = HD_IPC_SOCKET_PATH_FOR_CHILD
    };

    unlink(addr.sun_path);  // 确保 socket 文件不存在
    bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock_fd, 5);
     // 开启socket，子进程通过这个socket传递自己的数据，比如已启动。 // end


    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s] fork failed!\n", start_name, start_path);
        service->status = HD_SERVICE_STATUS_STOPPED;
        return -1;
    }
    else if (pid == 0)
    {
        // 子进程分支

        close(sock_fd);  // 子进程不需要监听
        int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
        // 执行新程序，并传递 socket 文件描述符
        char fd_str[10];
        snprintf(fd_str, sizeof(fd_str), "%d", client_fd);
      
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s]Child:[%d/%d] do execl... \n", start_name, start_path,  getppid(),getpid());
        int ret = execl(service->path, service->name, fd_str,NULL); // 成功时无返回值，失败返回 -1
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s]Child but Child execl() fail : %d! \n", start_name, start_path, ret);
        // exit(EXIT_FAILURE);
        _exit(ERROR_CHILD_START_FAIL); // 通知Parent
    }
    else
    {
        // 父进程分支
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] Parent:[%d/%d]! \n", start_name, start_path, getpid(), pid);
        service->status = HD_SERVICE_STATUS_STARTTING;
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

        /* 接受子进程的状态数据 */
        // 接受子进程返回给父进程表明启动成功 : <进程名称>,<进程id>,<程序版本号> 
        int client_fd = accept(sock_fd, NULL, NULL);
        char status[256];
        read(client_fd, status, sizeof(status));
        if (strlen(status)!=0)
        {
            char s_name[128];
            char s_version[128];
            int s_id;
            sscanf(status,"%127[^,],%d,%127[^,]",s_name,&s_id,s_version);
            HD_LOGGER_ERROR(TAG,"op_start_service_internal Parent received: <%s> <%s> <%d>\n", status,s_name,s_id);
            HDService *service = hd_service_array_find_by_name(&g_service_array,s_name);
            if (service==NULL){
                HD_LOGGER_ERROR(TAG,"op_start_service_internal service not found: %s\n", s_name);
            }else{
                if (service->pid == s_id){
                    stpncpy(service->version,s_version,strlen(s_version));
                    service->status = HD_SERVICE_STATUS_STARTED;
                    HD_LOGGER_ERROR(TAG,"op_start_service_internal %s SERVICE-STARTED!!! %s %s\n", s_name,PREFIX,PREFIX);
                    hd_service_array_print(&g_service_array);
                    kill(s_id,SIGUSR1);
                    if (strcmp(HD_INIT_SERVICE_MAIN,service->name) == 0)
                    {
                        start_main_service_count=0;
                    }
                    
                }else{
                    HD_LOGGER_ERROR(TAG,"op_start_service_internal pid not same : %d == %d = %d\n", service->pid,s_id,service->pid == s_id);
                }
            }  
        }else{
            HD_LOGGER_ERROR(TAG,"op_start_service_internal empty status !\n");
        }
        close(client_fd);
        close(sock_fd);
        unlink(addr.sun_path);  // 清理 socket 文件
        /* 接受子进程的状态数据 end */

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
        .status = HD_SERVICE_STATUS_STOPPED,
        .type = HD_SERVICE_TYPE_MAIN,
        .depends_on_count = 1,
        .version = "0.0.0",
        .depends_on = {HD_INIT_SERVICE_LOG}
    };
    hd_service_array_add(&g_service_array, &main_service);

    // 日志服务
    HD_LOGGER_INFO(TAG, "init_core_services[%s:%s] ...\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
    HDService log_service = {
        .name = HD_INIT_SERVICE_LOG,
        .path = HD_INIT_SERVICE_LOG_PATH,
        .status = HD_SERVICE_STATUS_STOPPED,
        .type = HD_SERVICE_TYPE_SECONDARY,
        .depends_on_count = 1,
        .version = "0.0.0",
        .depends_on = {}
    };
    hd_service_array_add(&g_service_array, &log_service);

    return 0;
}

static void ipc_list_services(int client_fd) {
    char buffer  [1024] ;
    hd_service_array_print_result(&g_service_array,buffer);
    write(client_fd, buffer, strlen(buffer));
}

static void ipc_show_service_detail(int client_fd,const char *service_name){
    const HDService *service  = hd_service_array_find_by_name(&g_service_array,service_name);
    if (service==NULL)
    {
        return ;
    }
     char buffer[4096];
    hd_service_print_string(service,buffer);
    write(client_fd, buffer, strlen(buffer));

}

void handle_client_command(int client_fd, int argc, const char *argv[]) {
    char buffer[1024];
    char result [2048];
    hd_printf_argc_argv(argc,argv,result);
    HD_LOGGER_DEBUG(TAG,"handle_client_command argc:%d ,result:%s\n",argc ,result);	
    if (argc <1) {
        snprintf(buffer, sizeof(buffer), "%s\n",ipc_print_help_str());
        printf("%s",buffer);
        write(client_fd, buffer, strlen(buffer));
        return;
    }

    if (strcmp(argv[0], "-v") == 0) {
        snprintf(buffer, sizeof(buffer), "hdinit version %s\n", VERSION);
        HD_LOGGER_DEBUG(TAG,"%s handle_client_command:%s \n",buffer ,PREFIX);
        write(client_fd, buffer, strlen(buffer));
    }  
    else if (strcmp(argv[0], "-h") == 0) {
        snprintf(buffer, sizeof(buffer),"%s",ipc_print_help_str());
        printf("%s",buffer);
        write(client_fd, buffer, strlen(buffer));
    }  
    else if (strcmp(argv[0], "-l") == 0) {
        ipc_list_services(client_fd);
    }    
    else if (strcmp(argv[0], "-d") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit -d <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        ipc_show_service_detail(client_fd, argv[1]);
    }

    else if (strcmp(argv[0], "start") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit start <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array,argv[1]);
        if (service==NULL)
        {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        
        if (HD_SERVICE_STATUS_STARTED == service->status ) {
            snprintf(buffer, sizeof(buffer), "Service %s is already running\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (0 == op_start_service_internal(service)) {
            snprintf(buffer, sizeof(buffer), "Started service %s\n", argv[1]);
        } else {
            snprintf(buffer, sizeof(buffer), "Failed to start service %s\n", argv[1]);
        }
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "stop") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit stop <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array,argv[1]);
        if (service == NULL) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (!(HD_SERVICE_STATUS_STARTED == service->status)) {
            snprintf(buffer, sizeof(buffer), "Service %s is not running\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (op_stop_service_internal(service) == 0) {
            snprintf(buffer, sizeof(buffer), "Stopped service %s\n", argv[1]);
        } else {
            snprintf(buffer, sizeof(buffer), "Failed to stop service %s\n", argv[1]);
        }
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "check") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit check <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array,argv[1]);
        if (service == NULL) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }

        if (op_check_service_update_internal(service)!=0) {
            snprintf(buffer, sizeof(buffer), "Service %s has updates\n", argv[1]);
        } else {
            snprintf(buffer, sizeof(buffer), "Service %s is up to date\n", argv[1]);
        }

        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "register") == 0) {
        if (argc < 3) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit register <service name> <path>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }

        HDService todo_service = {
            .status = HD_SERVICE_STATUS_STOPPED,
            .type = HD_SERVICE_TYPE_OTHER,
            .depends_on_count = 0,
            .version = "0.0.0",
            .depends_on = {0}
        };

        strcpy(todo_service.name, argv[1]);
        strcpy(todo_service.path, argv[2]);
        if ( hd_service_array_add(&g_service_array, &todo_service) == 0) {
            snprintf(buffer, sizeof(buffer), "Registered service %s at %s\n", argv[1], argv[2]);
        } else {
            snprintf(buffer, sizeof(buffer), "Failed to register service %s\n", argv[1]);
        }
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "unregister") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit unregister <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array,argv[1]);
        if (service == NULL) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }

        
        if (hd_service_array_remove_by_name(&g_service_array,service->name) == 0) {
            snprintf(buffer, sizeof(buffer), "Unregistered service %s\n", argv[1]);
        } else {
            snprintf(buffer, sizeof(buffer), "Failed to unregister service %s\n", argv[1]);
        }
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "shutdown") == 0) {
        snprintf(buffer, sizeof(buffer), "Shutting down all services...\n");
        write(client_fd, buffer, strlen(buffer));
        opt_reboot_internal();
        service_manager_running = 0;
        snprintf(buffer, sizeof(buffer), "Bye!\n");
        write(client_fd, buffer, strlen(buffer));
    }
    else {
        snprintf(buffer, sizeof(buffer), "不支持的命令:[%s]!使用hdinit -h查看所有命令。\n",argv[0]);
        write(client_fd, buffer, strlen(buffer));
    }
}

/**
 * 处理shell命令
 * 线程函数
 */
void *ipc_server_thread(void *arg)
{
    HD_LOGGER_DEBUG(TAG,"%s ipc_server_thread \n",PREFIX);
    const char * socket_path = HD_IPC_SOCKET_PATH;
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    unlink(socket_path);
    
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        HD_LOGGER_ERROR(TAG,"%s ipc_server_thread socket[%s] error : %d(%s)!\n",PREFIX,socket_path,errno,strerror(errno));
        perror("socket error");
        hd_init_exit();
        //exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, socket_path);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        HD_LOGGER_ERROR(TAG,"ipc_server_thread socket[%s] error bind: %d(%s)!\n",socket_path,errno,strerror(errno));
        hd_init_exit();
        //exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) == -1) {
        HD_LOGGER_ERROR(TAG,"ipc_server_thread socket[%s] error listen: %d(%s)!\n",socket_path,errno,strerror(errno));
        hd_init_exit();
        //exit(EXIT_FAILURE);
    }
    
    while (service_manager_running) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            if (service_manager_running) {
                HD_LOGGER_ERROR(TAG,"ipc_server_thread socket[%s] error accept: %d(%s)!\n",socket_path,errno,strerror(errno));
            }
            continue;
        }
        
        char cmd[1024];
        int n = read(client_fd, cmd, sizeof(cmd) - 1);
        if (n > 0) {
            cmd[n] = '\0';
            
            // 解析命令
            const char *argv[10];
            int argc = 0;
            char *token = strtok(cmd, " \n");
            
            while (token != NULL && argc < 10) {
                argv[argc++] = token;
                token = strtok(NULL, " \n");
            }
            if (argc > 0) {
                handle_client_command(client_fd, argc, argv);
            }
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    unlink(socket_path);
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
    HD_LOGGER_INFO(TAG, "start_core_services[%s:%s] ... \n", HD_INIT_SERVICE_MAIN, HD_INIT_SERVICE_MAIN_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_MAIN);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start_core_services[%s:%s]start fail!\n", HD_INIT_SERVICE_MAIN, HD_INIT_SERVICE_MAIN_PATH);
        return -1;
    }

    // 启动日志服务
    HD_LOGGER_INFO(TAG, "start_core_services[%s:%s] ...\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_LOG);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start_core_services[%s:%s]start fail!\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
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
        hd_init_exit();
        //exit(EXIT_FAILURE);
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
static int op_stop_service_internal( HDService *service)
{
    if (service == NULL)
    {
        return -1;
    }
    const char *s_name = service->name;
    const char *s_path = service->path;
    pid_t child_pid = service->pid;
    HD_LOGGER_ERROR(TAG, "op_stop_service_internal : %s %s ......\n", s_name, s_path);
    if (HD_SERVICE_STATUS_STARTED == service->status || HD_SERVICE_STATUS_STARTTING == service->status)
    {
        service->status = HD_SERVICE_STATUS_STOPPING;
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
    HD_LOGGER_INFO("hdlog","%s signal_handle_reboot %s \n",PREFIX,PREFIX);
    opt_reboot_internal();
}

/**
 * 检测服务状态
 */
static void monitor_services(){


    if (start_main_service_count>MAX_RESTART_MAIN)
    {
        hd_init_exit();
        return;
    }
    

    for (int i = 0; i < g_service_array.count; i++)
    {
        HDService *service = g_service_array.services+i; 
        
        if (service->status == HD_SERVICE_STATUS_STOPPED)
        {
            if (!service_manager_running)
            {
                break;
            }
            HD_LOGGER_DEBUG(TAG, "%-8s#monitor_services# [%s] %s ... \n",SPIT,service->name,hd_service_status_string(service->status));
            if (kill(service->pid, 0) != 0) // 不在运行中
            {
                op_start_service_internal(service); // 重新启动服务
            }
        }
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
        HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d ... \n",start_main_service_count,service_manager_running);
        if (!service_manager_running)
        {
            break;
        }
        
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
        HD_LOGGER_DEBUG(TAG, "#upgrade_services# ... \n");
        upgrade_services();
    }
    return NULL;
}

void handler_child_started(int sig) {
    printf("Received value: %d\n", sig);
}

static void hd_init_exit(){
    opt_reboot_internal();
}


static void hd_init_sigint_handler(int sig){
    HD_LOGGER_ERROR(TAG,"hd_init_sigint_handler sig = %d \n",sig);
    hd_init_exit();
}

/**
 *  gcc hd_init.c hd_logger.c hd_service.c -o hd_init -lpthread
 */
int main(int argc, char const *argv[])
{
    /*[注册信号]*/
    signal(SIGINT, hd_init_sigint_handler);  // 注册信号处理器 ctrl + c
    signal(SIGTSTP, hd_init_sigint_handler);  // 注册信号处理器 ctrl + z

    hd_logger_set_level(HD_LOGGER_LEVEL_INFO);

    HD_LOGGER_INFO(TAG, "================================================\n");
    HD_LOGGER_INFO(TAG, "Starting hd_init service manager (version:%d ) ...\n", VERSION);
    HD_LOGGER_INFO(TAG, "================================================\n");
    int ret;

    /* [初始化core服务] */
    HD_LOGGER_INFO(TAG, "[init_core_services ... ]\n");
    hd_service_array_init(&g_service_array);
    ret = init_core_services();
    hd_service_array_print(&g_service_array);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "[init_core_services error! ]\n");
        hd_init_exit();
      
        //exit(EXIT_FAILURE);
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

    /* [启动core服务] */
    HD_LOGGER_INFO(TAG, "[start_core_services size=%d... ]\n",g_service_array.count);
    ret = start_core_services();
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "[start_core_services error! ]\n");
        hd_init_exit();
        //exit(EXIT_FAILURE);
    }

    /* [启动monitor线程] */
    HD_LOGGER_INFO(TAG, "[monitor_services_thread_t started !!! ]\n");
    pthread_t monitor_services_thread_t;
    pthread_create(&monitor_services_thread_t,NULL,monitor_thread_services_thread,NULL);

    /* 启动upgrade线程 */
    HD_LOGGER_INFO(TAG, "[upgrade_services_thread_t started !!!] \n");
    pthread_t upgrade_services_thread_t;
    pthread_create(&upgrade_services_thread_t,NULL,upgrade_thread_services_thread,NULL);

    pthread_join(monitor_services_thread_t,NULL);
    HD_LOGGER_INFO(TAG, "[monitor_services_thread_t end !!!] \n");
    pthread_join(upgrade_services_thread_t,NULL);
    HD_LOGGER_INFO(TAG, "[upgrade_services_thread_t end !!!] \n");

    /* [hdinit运行结束] */
    HD_LOGGER_INFO(TAG, "[hdinit end!] \n");

    //printf("准备自杀...\n");
    //kill(getpid(), SIGKILL);  // SIGTERM 更温和
    //kill(getpid(), SIGTERM);  // SIGTERM 更温和
    //printf("这行不会执行\n");
    hd_service_array_cleanup(&g_service_array);
    return 0;
}
