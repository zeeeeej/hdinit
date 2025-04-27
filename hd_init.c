#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "hd_init.h"
#include <signal.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include "hd_logger.h"
#include "hd_utils.h"
#include "hd_ipc.h"
#include "hd_http.h"
#include "hd_utils.h"
#include "cJSON.h"
#include "hd_ipc_init.h"
#define PREFIX "######"

static void opt_reboot_internal();
static int op_stop_service_internal(HDService *service);
static int op_check_service_update_internal(HDService *service, hd_http_check_resp *resp);
static void hd_init_exit();
static int do_stop_service_by_name(const char *service_name);
static int upgrade_service(HDService *service);
static void progress_callback(double progress);
static int write_to_client(int client_fd, const char *buffer, size_t buffer_size);
static int hd_ipc_continu_update(int client_fd, HDService *service, const hd_http_check_resp *resp);
static void monitor_services();

static int g_blocking = 0;

typedef struct
{
    HDService *service;
    // const char *service_name;
} Wait_Child_Exit_Thread_Args;

#define _D_(msg, ...) HD_LOGGER_DEBUG(TAG, msg, ##__VA_ARGS__)


static void hide_terminal_cursor() {
    int tty_fd = open("/dev/tty0", O_WRONLY);
    if (tty_fd >= 0) {
        write(tty_fd, "\033[9;0]", 6);  // 发送ANSI控制序列
        close(tty_fd);
    } else {
        perror("Failed to open /dev/tty0");
    }
}

static int _start_qt(){
	// 1. 隐藏终端光标
 //   hide_terminal_cursor();

    // 2. 设置Qt环境变量
 //   char *env[] = {
 //       "QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1",
 //       "QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0",
 //       "QT_QPA_FONTDIR=/usr/lib/fonts/",
 //       NULL
 //   };

    // 3. 启动Qt程序
    pid_t pid = fork();
    if (pid == 0) { // 子进程
        execle("/root/LED_TEMP_HUMI", "./LED_TEMP_HUMI", NULL, NULL);
        perror("execle failed");
        exit(1);
    } else if (pid > 0) { // 父进程
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Qt程序退出码: %d\n", WEXITSTATUS(status));
        }
    } else {
        perror("fork failed");
        return 1;
    }
	
    return 0;

}

static const  char *env[] = {
        "QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1",
        "QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0",
        "QT_QPA_FONTDIR=/usr/lib/fonts/",
        NULL  // 必须以NULL结尾
    };



static int start_qt_real(){
	// 设置环境变量数组（注意：会完全替换子进程的环境变量）
    char *env[] = {
        "QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1",
        "QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0",
        "QT_QPA_FONTDIR=/usr/lib/fonts/",
        NULL  // 必须以NULL结尾
    };

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) { // 子进程
        
        execle("/root/LED_TEMP_HUMI", "/root/LED_TEMP_HUMI", (char *)NULL, env);
        // 只有exec失败才会执行到这里
        perror("execle failed");
        _exit(1);  // 使用_exit避免刷新stdio缓冲区
    } else { // 父进程
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        } else {
            printf("Child terminated abnormally\n");
            return 1;
        }
    }
    return 0;
}

static void * start_qt_thread(void * arg){
	start_qt_real();
	return NULL;
}

static int start_qt(){
	pthread_t t;
	pthread_create(&t,NULL,start_qt_thread,NULL);
	return 0;
}


/**
 * 主进程等待子进程退出
 * 线程函数
 */
void *wait_child_exit_thread(void *arg)
{
    // hd_service_array_print(&g_service_array);
    Wait_Child_Exit_Thread_Args *args = (Wait_Child_Exit_Thread_Args *)arg;
    pthread_t current_tid = pthread_self();
    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread arg=%s p:%p [%lu]\n", SPIT, args->service->name, args->service, (unsigned long)current_tid);
    hd_service_array_print(&g_service_array);
    HDService *service = args->service; // hd_service_array_find_by_name(&g_service_array, args->service_name);
    if (service == NULL)
    {
        free(args);
        return NULL;
    }

    char *s_name = service->name;
    char *s_path = service->path;
    pid_t pid = service->pid;
    free(args);

    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] wait exit ... \n", SPIT, s_name, s_path, getpid(), pid);

    int status;
    _D_("_debug_ wait_child_exit_thread  waitpid start.\n");
    waitpid(pid, &status, 0); // 等待子进程结束
    _D_("_debug_ wait_child_exit_thread  waitpid end.\n");
    if (!service_manager_running)
    {
        return NULL;
    }
    HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d]exited with status <%d|%d>\n", SPIT, s_name, s_path, getpid(), pid, status, WEXITSTATUS(status));

    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == ERROR_CHILD_START_FAIL)
        {
            HD_LOGGER_DEBUG(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] child execl() failed! %d \n", SPIT, s_name, s_path, getpid(), pid, start_main_service_count);
            if (strcmp(s_name, HD_INIT_SERVICE_MAIN) == 0)
            {
                start_main_service_count++;
                HD_LOGGER_ERROR(TAG, "%-8s@wait_child_exit_thread[%s:%s][%d/%d] hdmain execl() failed!start_main_service_count %d \n", SPIT, s_name, s_path, getpid(), pid, start_main_service_count);
            }
        }
    }

    // 更新service
    service->status = HD_SERVICE_STATUS_STOPPED;

    _D_("_debug_ wait_child_exit_thread  exit.\n");
    pthread_exit(NULL); // 显式终止当前线程
    return NULL;
}

/**
 * 启动服务
 */
static int op_start_service_internal(HDService *service)
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

    if (service->status == HD_SERVICE_STATUS_STARTED)
    {
        HD_LOGGER_DEBUG(TAG, "op_start_service_internal[%s:%s] already started!\n", service->name, service->path);
       return -1;
    }

    if (service->status == HD_SERVICE_STATUS_STARTTING)
    {
        HD_LOGGER_WARNING(TAG, "op_start_service_internal[%s:%s] starting ...\n", service->name, service->path);
       return -1;
    }
    
    char *start_name = service->name;
    char *start_path = service->path;
    HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] p->%p fork() ......\n", start_name, start_path, service);
    pid_t pid = fork();
    if (pid < 0)
    {   
        // fork失败
        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s] fork failed!\n", start_name, start_path);
        service->status = HD_SERVICE_STATUS_STOPPED;
        return -1;
    }
    else if (pid == 0)
    {
        // 子进程分支
        _D_("_debug_ op_start_service_internal child start.\n");
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s]Child:[%d/%d] do execl... \n", start_name, start_path, getppid(), getpid());
	int ret;
	if(strcmp(start_name,HD_INIT_SERVICE_QT)==0){
		ret = execl(service->path, service->name, NULL,env);
	}else{
	
        	ret = execl(service->path, service->name, NULL, NULL); // 成功时无返回值，失败返回 -1
	}

        HD_LOGGER_ERROR(TAG, "op_start_service_internal[%s:%s]Child but Child execl() fail : %d! \n", start_name, start_path, ret);
        // exit(EXIT_FAILURE);
        _exit(ERROR_CHILD_START_FAIL); // 通知Parent
        _D_("_debug_ op_start_service_internal child end.\n");
    }
    else
    {
        // 父进程分支
        if (!service_manager_running)
        {
            return -1;
        }
        _D_("_debug_ op_start_service_internal parent start.\n");
     
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] Parent:[%d/%d]! \n", start_name, start_path, getpid(), pid);
        service->status = HD_SERVICE_STATUS_STARTTING;
        service->pid = pid;
        service->last_modified = hd_get_last_modified(start_path);

        // 开启线程监听子进程结束 TODO 处理线程生命周期
        pthread_t wait_child_exit_thread_t;
        Wait_Child_Exit_Thread_Args *args = malloc(sizeof(Wait_Child_Exit_Thread_Args));
        HD_LOGGER_INFO(TAG, "op_start_service_internal[%s:%s] Parent arg:%s p:%p\n", start_name, start_path, service->name, service);
        args->service = service;
        pthread_create(&wait_child_exit_thread_t, NULL, wait_child_exit_thread, args);
        _D_("_debug_ op_start_service_internal parent end.\n");
        return 0;
    }
}

/**
 * 根据服务名称启动服务
 */
static int op_start_service_by_name(const char *service_name)
{   
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
        .depends_on = {HD_INIT_SERVICE_LOG}};
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
        .depends_on = {}};
    hd_service_array_add(&g_service_array, &log_service);

    HD_LOGGER_INFO(TAG, "init_core_services[%s:%s] ...\n", HD_INIT_SERVICE_RPC, HD_INIT_SERVICE_RPC_PATH);
    HDService rpc_service = {
        .name = HD_INIT_SERVICE_RPC,
        .path = HD_INIT_SERVICE_RPC_PATH,
        .status = HD_SERVICE_STATUS_STOPPED,
        .type = HD_SERVICE_TYPE_MAIN,
        .depends_on_count = 1,
        .version = "0.0.0",
        .depends_on = {HD_INIT_SERVICE_LOG}};
    hd_service_array_add(&g_service_array, &rpc_service);
    return 0;
}

static void ipc_list_services(int client_fd)
{
    char buffer[2048];
    hd_service_array_print_result(&g_service_array, buffer);
    write_to_client(client_fd, buffer, strlen(buffer));
}

static void ipc_show_service_detail(int client_fd, const char *service_name)
{
    const HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    if (service == NULL)
    {
        return;
    }
    char buffer[4096];
    hd_service_print_string(service, buffer);
    write_to_client(client_fd, buffer, strlen(buffer));
}

static void ipc_check_update(int client_fd, const char *service_name)
{
    //    pthread_t t;
    //    struct ipc_check_update_thread_data data = {
    //         .client_fd = client_fd,
    //         .service_name = (char*)service_name
    //    };
    //    pthread_create(&t,NULL,ipc_check_update_thread,&data);
    //    pthread_join(t,NULL);
    //    printf("================ over ====================\n");

    // struct  ipc_check_update_thread_data  * data  = (struct ipc_check_update_thread_data *) arg;
    // char * service_name  = data->service_name;
    // int client_fd = data->client_fd;
    char buffer[2048];
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    if (service == NULL)
    {
        snprintf(buffer, sizeof(buffer), "Service %s not found\n", service_name);
        write_to_client(client_fd, buffer, strlen(buffer));
        return;
    }
    int ret = upgrade_service(service);

    // hd_http_check_resp resp;
    // if (op_check_service_update_internal(service,&resp)>0) {
    //     snprintf(buffer, sizeof(buffer), "Service %s has updates\n", argv[1]);
    // } else {
    //     snprintf(buffer, sizeof(buffer), "Service %s is up to date\n", argv[1]);
    // }

    if (ret == 0)
    {
        snprintf(buffer, sizeof(buffer), "Service %s 更新完毕！（%d）\n\r", service_name, ret);
    }
    else if (ret == -2)
    {
        snprintf(buffer, sizeof(buffer), "Service %s 正在更新中...（%d）\n\r", service_name, ret);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "Service %s 无更新！（%d）\n\r", service_name, ret);
    }
    write_to_client(client_fd, buffer, strlen(buffer));
    write_to_client(client_fd, "================ ok ====================", strlen("================ ok ===================="));
    printf("================ ok2 ====================\n");
}

static int write_to_client(int client_fd, const char *buffer, size_t buffer_size)
{


    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(client_fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) 
    {
        if (error == EPIPE || error == ECONNRESET)
        {
            printf("Connection is broken.\n");
            close(client_fd);
            return -1;
        } 
        else 
        {
            
            struct timespec req = {0, 200000000}; // 0秒 + 200,000,000 纳秒 = 200ms
            nanosleep(&req, NULL);
            ssize_t size  = write(client_fd, buffer, buffer_size);
            if ((size_t)size  == buffer_size)
            {
                printf("write() Successfully sent %zd bytes.\n", size);
            }
            else if(size > 0)
            {
                printf("write(): %zd/%zu bytes sent.\n", size, buffer_size);
            }
            else
            {
                perror("write() failed");
                close(client_fd);
                return -1;
            }
            return 0 ;
        }
    
    }
    else
    {
        return -1;
    }

    
}

static int ipc_write_json_print_internal(int client_fd, const char *msg)
{
    char result[HD_IPC_SOCKET_PATH_BUFF_SIZE];
    HD_LOGGER_DEBUG(TAG, "%s\n", msg);
    int ret = hd_ipc_json_print(msg, result);
    if (ret != 0)
    {
        HD_LOGGER_ERROR(TAG, "ipc构建json失败\n");
        return -1;
    }
    return write_to_client(client_fd, result, strlen(result));
}

static int  ipc_write_json_progress_internal(int client_fd, int progress)
{
    char result[HD_IPC_SOCKET_PATH_BUFF_SIZE];
    int ret = hd_ipc_json_resp_progress(progress, result);
    if (ret != 0)
    {
        HD_LOGGER_ERROR(TAG, "ipc构建json失败\n");
        return -1;
    }
   return  write_to_client(client_fd, result, strlen(result));
}


static void * op_start_service_internal_thread(void * arg){
    char * service_name = (char * )arg;
    if (service_name==NULL)
    {
        HD_LOGGER_ERROR(TAG, "<op_start_service_internal_thread>service_name is null!\n");
       return NULL;
    }
    
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    free(service_name);
    op_start_service_internal(service);
    return NULL;
}

/**
 * 根据服务名称启动服务
 */
static int op_start_service_by_shell(const char *service_name)
{   
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    op_start_service_internal(service);

    // 用线程没效果 还是会阻塞住shell。
    // g_blocking = 1;
    // pthread_t t ;
    // pthread_create(&t,NULL,op_start_service_internal_thread,strdup(service_name));
    // while (g_blocking)
    // {
    //     sleep(1);//等待处理结果
    // }
    return 0;
}

/**
 * 1.客户端发送完命令 开启阻塞 等待服务器处理命令。
 * 2.超时时间 可选
 * 3.服务器需要异步发送自己的状态 在合适的时机结束阻塞。
 * 4.
 */
static void handle_client_command(int client_fd, int argc, const char *argv[])
{
    char buffer[2048];
    char result[2048];
    char s_out[2048];
    hd_printf_argc_argv(argc, argv, result);
    HD_LOGGER_INFO(TAG, "handle_client_command argc:%d ,result:%s\n", argc, result);
    if (argc < 1)
    {
        snprintf(buffer, sizeof(buffer), "%s\n", ipc_print_help_str());
        printf("%s", buffer);
        write_to_client(client_fd, buffer, strlen(buffer));
        return;
    }

    if (strcmp(argv[0], "-v") == 0)
    {
        snprintf(buffer, sizeof(buffer), "hdinit version %s\n", VERSION);
        HD_LOGGER_DEBUG(TAG, "%s handle_client_command:%s \n", buffer, PREFIX);
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "-h") == 0)
    {
        snprintf(buffer, sizeof(buffer), "%s", ipc_print_help_str());
        printf("%s", buffer);
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "-l") == 0)
    {
        ipc_list_services(client_fd);
    }
    else if (strcmp(argv[0], "-d") == 0)
    {
        if (argc < 2)
        {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit -d <service name>\n");
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        ipc_show_service_detail(client_fd, argv[1]);
    }

    else if (strcmp(argv[0], "start") == 0)
    {
        if (argc < 2)
        {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit start <service name>\n");
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array, argv[1]);
        if (service == NULL)
        {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }

        if (HD_SERVICE_STATUS_STARTED == service->status)
        {
            snprintf(buffer, sizeof(buffer), "Service %s is already running\n", argv[1]);
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        if (0 == op_start_service_by_shell(service->name))
        {
            snprintf(buffer, sizeof(buffer), "Started service %s\n", argv[1]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Failed to start service %s\n", argv[1]);
        }
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        if (argc < 2)
        {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit stop <service name>\n");
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array, argv[1]);
        if (service == NULL)
        {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        if (!(HD_SERVICE_STATUS_STARTED == service->status))
        {
            snprintf(buffer, sizeof(buffer), "Service %s is not running\n", argv[1]);
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        op_stop_service_internal(service);

        sleep(1);
        int ret = service->status == HD_SERVICE_STATUS_STOPPED || service->status == HD_SERVICE_STATUS_STOPPING;
        if (ret)
        {
            snprintf(buffer, sizeof(buffer), "Stopped service %s\n", argv[1]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Failed to stop service %s\n", argv[1]);
        }
        write_to_client(client_fd, buffer, strlen(buffer));
    }

    else if (strcmp(argv[0], "check") == 0)
    { // start check
        int check_ret;
        char check_tmp[2048] = {0};
        char check_result[2048] = {0};
        hd_http_check_resp resp;
        char destPath[1024] = {0};
        char ota_path[1024] = {0};
        /**
         *  check <hdmain> [-y]
         */
        if (argc < 2)
        {
            snprintf(check_tmp, sizeof(check_tmp), "Usage: hdinit check <service name> [-y]\n");
            ipc_write_json_print_internal(client_fd, check_tmp);
            return;
        }

        // 原来实现
        // ipc_check_update(client_fd,argv[1]);

        // 新实现
        // 1.提取参数
        int auto_update = 0;
        if (3 == argc)
        {
            if (
                (strcmp(argv[2], "-y") == 0) || (strcmp(argv[2], "-Y") == 0) ||
                (strcmp(argv[2], "-yes") == 0) || (strcmp(argv[2], "-YES") == 0))
            {
                auto_update = 1;
            }
        }
        // 2.检查service
        snprintf(check_tmp, sizeof(check_tmp), "[%s] Check Service ...\n", argv[1]);
        ipc_write_json_print_internal(client_fd, check_tmp);

        HDService *service = hd_service_array_find_by_name(&g_service_array, argv[1]);
        if (service == NULL)
        {
            snprintf(check_tmp, sizeof(check_tmp), "[%s] Service Not Found!！\n", argv[1]);
            ipc_write_json_print_internal(client_fd, check_tmp);
            return;
        }
        snprintf(check_tmp, sizeof(check_tmp), "[%s] Service Found!！ \n-name:%s\n-pid:%d\n-version:%s\n", argv[1], service->name, service->pid, service->version);
        ipc_write_json_print_internal(client_fd, check_tmp);

        // 3.检查更新
        snprintf(check_tmp, sizeof(check_tmp), "[%s] Check Version  ...\n", argv[1]);
        ipc_write_json_print_internal(client_fd, check_tmp);
        if (op_check_service_update_internal(service, &resp) > 0)
        {
            // ota地址
            snprintf(ota_path, sizeof(ota_path), "%s/ota/%s", HD_INIT_ROOT, resp.filename);

            snprintf(check_tmp, sizeof(check_tmp), "[%s](%s) Has New Version!!! \n-version:%s\n-url:%s\n-md5:%s\n-filename:%s\n", argv[1], service->version, resp.version, resp.url, resp.md5, resp.filename);
            ipc_write_json_print_internal(client_fd, check_tmp);

            if (auto_update)
            {
                service->update = 1;
                check_ret = hd_ipc_continu_update(client_fd, service, &resp);
                service->update = 0;
                return;
            }

            snprintf(check_tmp, sizeof(check_tmp), "输入y继续\n");
            ipc_write_json_print_internal(client_fd, check_tmp);

            // 需要更新
            check_ret = hd_ipc_json_resp_check_result(resp.url, resp.md5, resp.filename, service->name, resp.version, check_result);
            HD_LOGGER_INFO(TAG, "hd_ipc_json_resp_check_result : %d %s\n", check_ret, check_result);
            if (check_ret != 0)
                return;
            hd_print_buffer(check_result, strlen(check_result));
            write_to_client(client_fd, check_result, strlen(check_result));

            // 阻塞等待客户端回复
            ssize_t recv_bytes = recv (client_fd, buffer, sizeof(buffer), 0);
            if (recv_bytes > 0)
            {
                // 7. 检查输入是否为 y
                if (buffer[0] == 'y' || buffer[0] == 'Y')
                {
                    HD_LOGGER_DEBUG(TAG, "客户端确认，继续执行...\n");
                    // 后续逻辑...
                    service->update = 1;
                    check_ret = hd_ipc_continu_update(client_fd, service, &resp);
                    service->update = 0;
                }
                else
                {
                    HD_LOGGER_DEBUG(TAG, "客户端取消操作。\n");
                }
                }
            else if (recv_bytes == 0)
            {
                HD_LOGGER_INFO(TAG, "recv() Connection closed by client.\n");
                return;
            }
            else
            {
                perror("recv() failed!");
                return;
            }
            


        }
        else
        {
            snprintf(check_tmp, sizeof(check_tmp), "[%s] No Update ！\n", argv[1]);
            ipc_write_json_print_internal(client_fd, check_tmp);
        }
    } // end check

    else if (strcmp(argv[0], "register") == 0)
    {
        if (argc < 3)
        {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit register <service name> <path>\n");
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }

        HDService todo_service = {
            .status = HD_SERVICE_STATUS_STOPPED,
            .type = HD_SERVICE_TYPE_OTHER,
            .depends_on_count = 0,
            .version = "0.0.0",
            .depends_on = {0}};

        strcpy(todo_service.name, argv[1]);
        strcpy(todo_service.path, argv[2]);
        if (hd_service_array_add(&g_service_array, &todo_service) == 0)
        {
            snprintf(buffer, sizeof(buffer), "Registered service %s at %s\n", argv[1], argv[2]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Failed to register service %s\n", argv[1]);
        }
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "unregister") == 0)
    {
        if (argc < 2)
        {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit unregister <service name>\n");
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }
        HDService *service = hd_service_array_find_by_name(&g_service_array, argv[1]);
        if (service == NULL)
        {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write_to_client(client_fd, buffer, strlen(buffer));
            return;
        }

        if (hd_service_array_remove_by_name(&g_service_array, service->name) == 0)
        {
            snprintf(buffer, sizeof(buffer), "Unregistered service %s\n", argv[1]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Failed to unregister service %s\n", argv[1]);
        }
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "shutdown") == 0)
    {
        snprintf(buffer, sizeof(buffer), "Shutting down all services...\n");
        write_to_client(client_fd, buffer, strlen(buffer));
        opt_reboot_internal();
        service_manager_running = 0;
        snprintf(buffer, sizeof(buffer), "Bye!\n");
        write_to_client(client_fd, buffer, strlen(buffer));
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "不支持的命令:[%s]!使用hdinit -h查看所有命令。\n", argv[0]);
        write_to_client(client_fd, buffer, strlen(buffer));
    }
}

static int hd_ipc_continu_update(int client_fd, HDService *service, const hd_http_check_resp *resp)
{
    char tmp[4096] = {0};
    char bak_path[1024] = {0};
    char new_path[1024] = {0};
    int ret;

    // 通知开始下载
    snprintf(new_path, 1024, "%s/ota/%s", HD_INIT_ROOT, resp->filename);

    snprintf(tmp, sizeof(tmp), "[%s] Downloading ...\n", service->name);
    ipc_write_json_print_internal(client_fd, tmp);

    snprintf(tmp, sizeof(tmp), "from <%s> \n",resp->url);
    ipc_write_json_print_internal(client_fd, tmp);

    snprintf(tmp, sizeof(tmp), "to <%s> \n",new_path);
    ipc_write_json_print_internal(client_fd, tmp);

    ret = hd_http_download(resp->url, new_path, progress_callback);
    //hd_print_progress_bar(0);
    for (int i = 0; i <= 100; i++) {
        //hd_print_progress_bar(i);
        if(0!=ipc_write_json_progress_internal(client_fd, i))break;
        struct timespec req = {0, 10000000}; // 0秒 + 200,000,000 纳秒 = 200ms
        nanosleep(&req, NULL);
    }
    if(0!=ipc_write_json_print_internal(client_fd, "\nDone!\n")) return -4;
    
    if (ret == 0)
    {
        snprintf(tmp, sizeof(tmp), "[%s] Download success!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        // 备份老的程序
        snprintf(bak_path, 1024, "%s/bak/%s-%s", HD_INIT_ROOT, hd_get_filename(service->path), service->version);

        snprintf(tmp, sizeof(tmp), "[%s] Backup Old ... \n%s->%s\n", service->name, service->path, bak_path);
        ipc_write_json_print_internal(client_fd, tmp);

        ret = hd_cp_file(service->path, bak_path);
        if (ret)
        {
            snprintf(tmp, sizeof(tmp), "[%s] Bakup Old Fail! \n", service->name);
            ipc_write_json_print_internal(client_fd, tmp);
            return -3;
        }

        snprintf(tmp, sizeof(tmp), "[%s] Bakup Old Success!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        // 停止服务
        snprintf(tmp, sizeof(tmp), "[%s] Stop Service  ...\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        op_stop_service_internal(service);

        sleep(5);

        snprintf(tmp, sizeof(tmp), "[%s] Stop Service Completed!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        // 复制新程序
        snprintf(tmp, sizeof(tmp), "[%s] Update New ...\n%s->%s\n", service->name, new_path, service->path);
        ipc_write_json_print_internal(client_fd, tmp);

        ret = hd_cp_file(new_path, service->path);
        if (ret)
        {
            snprintf(tmp, sizeof(tmp), "[%s] Update New Fail! \n", service->name);
            ipc_write_json_print_internal(client_fd, tmp);
            return -4;
        }

        sleep(1);
        snprintf(tmp, sizeof(tmp), "[%s] Update New Success!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        // 启动服务
        snprintf(tmp, sizeof(tmp), "[%s] Start Service ...\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        op_start_service_internal(service);

        snprintf(tmp, sizeof(tmp), "[%s] Start Service Completed!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        snprintf(tmp, sizeof(tmp), "[%s] Completed!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);

        return 0;
    }
    else
    {
        snprintf(tmp, sizeof(tmp), "[%s] Download fail!\n", service->name);
        ipc_write_json_print_internal(client_fd, tmp);
        return -1;
    }
    return 0;
}

/**
 * Deprecated
 */
static int hd_ipc_json_handle(int client_fd, char *buffer, size_t buffer_size)
{
    // 解析 JSON 字符串
    // hd_print_buffer(buffer,size);
    char cmd[128] = {0};
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            // fprintf(stderr, "JSON parse error>>>: %s\n", error_ptr);
        }
        return -1;
    }

    // 提取 "cmd" 字段
    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (cmd_item == NULL || !cJSON_IsString(cmd_item))
    {
        fprintf(stderr, "Failed to parse 'cmd' field\n");
        cJSON_Delete(root);
        return -1;
    }

    if (strcmp("check", cmd) == 0)
    {
        // 升级
        /*
    {
        "cmd": "check",
        "param": {
            "url": "xxxxxxx",
            "md5":"xxx",
            "filename":"",
            "service":"hdmain",
            "version":"1.0.0"
        }
    }
    */

        return -1;
    }
    return 0;
}

/**
 * 处理shell命令
 * 线程函数
 */
void *ipc_server_thread(void *arg)
{
    HD_LOGGER_DEBUG(TAG, "%s ipc_server_thread \n", PREFIX);
    const char *socket_path = HD_IPC_SOCKET_PATH;
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    unlink(socket_path);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        HD_LOGGER_ERROR(TAG, "%s ipc_server_thread socket[%s] error : %d(%s)!\n", PREFIX, socket_path, errno, strerror(errno));
        perror("socket error");
        hd_init_exit();
        // exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, socket_path);

    // fcntl(server_fd, F_SETFL, O_NONBLOCK);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        HD_LOGGER_ERROR(TAG, "ipc_server_thread socket[%s] error bind: %d(%s)!\n", socket_path, errno, strerror(errno));
        hd_init_exit();
        // exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1)
    {
        HD_LOGGER_ERROR(TAG, "ipc_server_thread socket[%s] error listen: %d(%s)!\n", socket_path, errno, strerror(errno));
        hd_init_exit();
        // exit(EXIT_FAILURE);
    }

    while (service_manager_running)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1)
        {
            if (service_manager_running)
            {
                HD_LOGGER_ERROR(TAG, "ipc_server_thread socket[%s] error accept: %d(%s)!\n", socket_path, errno, strerror(errno));
            }
            continue;
        }
        HD_LOGGER_ERROR(TAG, "ipc_server_thread socket accept client : [%d] \n", client_fd);

        char cmd[1024];
        char json_buffer[1024];
        int json_ret = 0;
        int n = read(client_fd, cmd, sizeof(cmd) - 1);
        if (n > 0)
        {

            /* cJSON a*/
            json_ret = hd_ipc_json_handle(client_fd, cmd, sizeof(cmd));

            /* cJSON z*/

            if (json_ret == 0)
            {
                // hd_ipc_json_handle处理完毕
                continue;
            }

            cmd[n] = '\0';

            // 解析命令
            const char *argv[10];
            int argc = 0;
            char *token = strtok(cmd, " \n");

            while (token != NULL && argc < 10)
            {
                argv[argc++] = token;
                token = strtok(NULL, " \n");
            }
            if (argc > 0)
            {
                handle_client_command(client_fd, argc, argv);
            }
        }else{
            HD_PRINT_ERROR("hd_init.c","ipc_server_thread","read fail %d \n",n);
        }
        int ret = close(client_fd);
        HD_LOGGER_ERROR(TAG, "ipc_server_thread socket:[%d] close_client[%d] ret = %d \n", server_fd, client_fd, ret);
    }
    HD_LOGGER_ERROR(TAG, "ipc_server_thread socket:[%d] closed!!! \n", server_fd);
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
    sleep(15);
    // service_manager_running = 0;
    kill(getpid(), SIGUSR1);
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
	
    sleep(1);

    // 启动日志服务
    HD_LOGGER_INFO(TAG, "start_core_services[%s:%s] ...\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_LOG);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start_core_services[%s:%s]start fail!\n", HD_INIT_SERVICE_LOG, HD_INIT_SERVICE_LOG_PATH);
        return -1;
    }
    sleep(1);

    HD_LOGGER_INFO(TAG, "start_core_services[%s:%s] ...\n", HD_INIT_SERVICE_RPC, HD_INIT_SERVICE_RPC_PATH);
    ret = op_start_service_by_name(HD_INIT_SERVICE_RPC);
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "start_core_services[%s:%s]start fail!\n", HD_INIT_SERVICE_RPC, HD_INIT_SERVICE_RPC_PATH);
        return -1;
    }
    sleep(1);	
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
        // exit(EXIT_FAILURE);
    }
    return 0;
}

static int op_check_service_update_internal(HDService *service, hd_http_check_resp *resp)
{
    /* 服务器version */
    if (hd_http_check_update(service->name, resp) != 0)
    {
        HD_LOGGER_INFO(TAG, "无需更新 %s", service->name);
        return -1;
    }

    HD_LOGGER_INFO(TAG, "Update available:\n");
    HD_LOGGER_INFO(TAG, "URL:        %s\n", resp->url);
    HD_LOGGER_INFO(TAG, "Version:    %s\n", resp->version);
    HD_LOGGER_INFO(TAG, "MD5:        %s\n", resp->md5);
    HD_LOGGER_INFO(TAG, "Filename:   %s\n", resp->filename);

    /* 当前version */
    int ret = strcmp(resp->version, service->version) > 0;
    HD_LOGGER_INFO(TAG, "Current Version:  [%s > %s = %d]\n", resp->version, service->version, ret);
    return ret;
}

/**
 * 停止服务
 */
static int op_stop_service_internal(HDService *service)
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
            HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s (child_pid, SIGTERM) ==0 \n", s_name, s_path);
            sleep(1); // 给子进程时间清理
            int status;

            // 检查子进程是否还在运行
            if (waitpid(child_pid, &status, WNOHANG) == 0)
            {
                // 如果还在运行，强制终止
                kill(child_pid, SIGKILL);
                // waitpid(child_pid, &status, 0);
                // service->is_running = 0;
                HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s exit0 kill with SIGKILL! \n", s_name, s_path);
                return 0;
            }
            else
            {
                HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s exit0 kill failed! \n", s_name, s_path);
                return 0;
            }
            HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s exit with SIGTERM \n", s_name, s_path);
            return 0;
        }
        else
        {
            HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s exit with SIGTERM not. \n", s_name, s_path);
        }
    }
    HD_LOGGER_ERROR(TAG, "op_stop_service_internal kill: %s %s exit2 with already stopped! \n", s_name, s_path);
    return 0;
}

/**
 * 根据服务名称停止服务
 */
static int do_stop_service_by_name(const char *service_name)
{
    HDService *service = hd_service_array_find_by_name(&g_service_array, service_name);
    return op_stop_service_internal(service);
}

/**
 * 停止所有服务
 * 重启
 */
static void opt_reboot_internal()
{
    HD_LOGGER_DEBUG(TAG, "opt_reboot_internal !!! \n");
    sync(); // 确保数据写入磁盘
    sleep(1);
    HD_LOGGER_DEBUG(TAG, "opt_reboot_internal !!! synced \n");
    for (int i = 0; i < g_service_array.count; i++)
    {
        op_stop_service_internal(&g_service_array.services[i]);
    }
    service_manager_running = 0;

    HD_LOGGER_DEBUG(TAG, "opt_reboot_internal !!! all-stopped \n");
    // sleep(5); // 给时间保存数据
    // HD_LOGGER_DEBUG(TAG,"opt_reboot_internal !!! kill...\n");
    // kill(getpid(),SIGKILL);
    // HD_LOGGER_DEBUG(TAG,"opt_reboot_internal !!! killed \n");
    sleep(3);

    // reboot(RB_AUTOBOOT);  // 发起重启
    hd_trigger_reboot();
    sleep(3);
    exit(0);
}

/**
 * 处理重启信号
 */
void signal_handle_reboot(int sig)
{
    HD_LOGGER_INFO("hdlog", "%s signal_handle_reboot %s \n", PREFIX, PREFIX);
    opt_reboot_internal();
}

/**
 * 检测服务状态
 */
static void monitor_services()
{

    if (start_main_service_count >= MAX_RESTART_MAIN)
    {
        hd_init_exit();
        return;
    }

    for (int i = 0; i < g_service_array.count; i++)
    {
        HDService *service = g_service_array.services + i;

        if (service->status == HD_SERVICE_STATUS_STOPPED && service->update == 0)
        {
            if (!service_manager_running)
            {
                break;
            }
            HD_LOGGER_DEBUG(TAG, "%-8s#monitor_services# [%s] %s ... \n", SPIT, service->name, hd_service_status_string(service->status));
            if (kill(service->pid, 0) != 0) // 不在运行中
            {
                op_start_service_internal(service); // 重新启动服务
            }
        }
    }
}

static void progress_callback(double progress)
{
    HD_LOGGER_INFO(TAG, "-->progress:%d %%", progress);

    // // pthread_t current_tid = pthread_self();
    // // printf("progress_callback: %f \n",progress);
    // pthread_t t;

    // pthread_mutex_lock(&lock);
    hd_print_progress_double(progress);

    // pthread_mutex_unlock(&lock);

    // // pthread_mutex_lock(&pd.lock);
    // // if (*progress - pd.last_print >= 1.0 || *progress >= 100.0) {
    // //     hd_print_progress_double( *progress);
    // //     pd.last_print = *progress;
    // // }
    // // pthread_mutex_unlock(&pd.lock)
}

static int upgrade_service(HDService *service)
{
    if (service->update)
    {
        return -2;
    }

    /* 服务器version */
    hd_http_check_resp resp;
    char buff[1024];
    char destPath[1024];
    char tmp[1024];
    int ret;

    HD_LOGGER_DEBUG(TAG, "%-8s#upgrade_services# [%s] version:%s ... \n", SPIT, service->name, service->version);
    if (op_check_service_update_internal(service, &resp) > 0)
    {
        HD_LOGGER_INFO(TAG, "%s need update ！ %s->%s\n", service->name, service->version, resp.version);
        snprintf(buff, 1024, "%s/ota/%s", HD_INIT_ROOT, resp.filename);
        ret = hd_http_download(resp.url, buff, progress_callback);

        HD_LOGGER_INFO(TAG, "[update]download completed!!!\n");

        // {
        //      "md5": "4da0413eeeb4e45661cc7f6fa4a1bdc5",
        //      "url": "http://127.0.0.1:5000/files/hdmain-0.0.2",
        //      "version": "0.0.2"
        // }

        // hd_print_progress_bar(0);
        // for (int i = 0; i <= 100; i++) {
        //     hd_print_progress_bar(i);
        //     usleep(100000); // 延迟 100ms（微秒）
        // }
        // printf("\nDone!\n");

        if (ret == 0)
        {
            HD_LOGGER_INFO(TAG, "[update]%s download success from [%s] ！file path is :[%s] \n", service->name, resp.url, buff);
            /* 备份老的程序 */
            snprintf(destPath, 1024, "%s/bak/%s-%s", HD_INIT_ROOT, hd_get_filename(service->path), service->version);
            ret = hd_cp_file(service->path, destPath);
            if (ret)
            {
                HD_LOGGER_INFO(TAG, "[update]bakup fail! %s \n", service->name);
                return -3;
            }
            HD_LOGGER_INFO(TAG, "[update]%s backup complete!!! %s -> %s\n", service->path, destPath);

            /* 停止服务 */
            op_stop_service_internal(service);
            HD_LOGGER_INFO(TAG, "[update]%s stop-service complete!!! \n");
            sleep(5);

            /* 复制新程序   */
            ret = hd_cp_file(buff, service->path);
            if (ret)
            {
                HD_LOGGER_INFO(TAG, "[update]update fail! %s \n", service->name);
                return -4;
            }
            HD_LOGGER_INFO(TAG, "[update]%s update complete!!! %s -> %s\n", buff, service->path);
            service->update = 1;

            /* 启动服务 */
            HD_LOGGER_INFO(TAG, "[update]%s sleep 5s ... !!! %s -> %s\n");
            op_start_service_internal(service);
            HD_LOGGER_INFO(TAG, "[update]%s start-service  !!! %s -> %s\n");
            // service->update = 0;
            return 0;
        }
        else
        {
            HD_LOGGER_INFO(TAG, "[update]%s download fail ！ %s\n", service->name, resp.url);
            return -5;
        }

        HD_LOGGER_INFO(TAG, "[update]%s not need update !!! @%s \n", service->name, resp.url);
        return -3;
    }
    return -3;
}

/**
 * 检测服务升级
 */
static void upgrade_services()
{
    if (!service_manager_running)
    {
        return;
    }

    for (int i = 0; i < g_service_array.count; i++)
    {
        if (!service_manager_running)
        {
            return;
        }
        upgrade_service(g_service_array.services + i);
    }
}

void *monitor_thread_services_thread(void *arg)
{
    /* 开始循环 检测核心服务是否运行 */
    while (service_manager_running)
    {
        sleep(MONITOR_SERVICES_INTERVAL);
        HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d ... \n", start_main_service_count, service_manager_running);
        if (!service_manager_running)
        {
            break;
        }

        monitor_services();
        service_manager_running_count++;
    }
    return NULL;
}

void *monitor_thread_services_thread_by_select(void *arg)
{
    int retval;
    struct timeval tv;
    /* 开始循环 检测核心服务是否运行 */
    while (service_manager_running)
    {
        HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d ... \n", start_main_service_count, service_manager_running);
        tv.tv_sec = MONITOR_SERVICES_INTERVAL;
        tv.tv_usec = 0;
        retval = select(0, NULL, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("select()出错");
            HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d select出错 \n", start_main_service_count, service_manager_running);
        }
        else if (retval)
        {
            HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d select有输入 \n", start_main_service_count, service_manager_running);
        }
        else
        {
            HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d select没有输入 \n", start_main_service_count, service_manager_running);
            monitor_services();
            service_manager_running_count++;
        }
    }
    HD_LOGGER_DEBUG(TAG, "#monitor_services# %d-%d 任务结束 \n", start_main_service_count, service_manager_running);
    return NULL;
}

void *upgrade_thread_services_thread(void *arg)
{
    /* 开始循环 检测核心服务是否需要升级 */
    while (service_manager_running)
    {
        sleep(UPGRADE_SERVICES_INTERVAL);
        HD_LOGGER_DEBUG(TAG, "#upgrade_services# ... \n");
        upgrade_services();
    }
    return NULL;
}

void *upgrade_thread_services_thread_by_select(void *arg)
{

    /* 开始循环 检测核心服务是否需要升级 */
    while (service_manager_running)
    {
        int retval;
        struct timeval tv;
        HD_LOGGER_DEBUG(TAG, "##upgrade_services## %d-%d ... \n", start_main_service_count, service_manager_running);
        tv.tv_sec = UPGRADE_SERVICES_INTERVAL;
        tv.tv_usec = 0;
        retval = select(0, NULL, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("select()出错");
            HD_LOGGER_DEBUG(TAG, "##upgrade_services## %d-%d select出错 \n", start_main_service_count, service_manager_running);
        }
        else if (retval)
        {
            HD_LOGGER_DEBUG(TAG, "##upgrade_services## %d-%d select有输入 \n", start_main_service_count, service_manager_running);
        }
        else
        {
            HD_LOGGER_DEBUG(TAG, "##upgrade_services## %d-%d select没有输入 \n", start_main_service_count, service_manager_running);
            upgrade_services();
        }
    }
    HD_LOGGER_DEBUG(TAG, "##upgrade_services## %d-%d 任务结束 \n", start_main_service_count, service_manager_running);
    return NULL;
}

void handler_child_started(int sig)
{
    printf("Received value: %d\n", sig);
}

static void hd_init_exit()
{
    opt_reboot_internal();
}

static void ipc_resp_cmd_ipc_core_heartbeat_pong(const char * service_name,int index) {
}

static void ipc_resp_cmd_ipc_shell_confirm_upgrade_resp(const char * service_name) {

}

static void ipc_init_on_heartbeat_lost_internal(const char * name,int index,time_t time){
    HDService *service =  hd_service_array_find_by_name(&g_service_array,name);
    if (service==NULL)
    {
       return;
    }
    
    if (service->status != HD_SERVICE_STATUS_STARTED)
    {
        op_stop_service_internal(service);
    	if (strcmp(HD_INIT_SERVICE_RPC, name) == 0){
    		printf(" -------*------> on_rpc_service_exit\n");
    		printf(" -------*------> on_rpc_service_exit\n");
    		printf(" -------*------> on_rpc_service_exit\n");
    		printf(" -------*------> on_rpc_service_exit\n");
    		printf(" -------*------> on_rpc_service_exit\n");
    		printf(" -------*------> on_rpc_service_exit\n");
       		do_stop_service_by_name(HD_INIT_SERVICE_MQTT);
        	do_stop_service_by_name(HD_INIT_SERVICE_UART);
	 	do_stop_service_by_name(HD_INIT_SERVICE_QT);

    	}
    }

    

}


static  void ipc_init_on_connected_internal(const char * service_name,int service_pid,const char * service_version){
    HDService *service =  hd_service_array_find_by_name(&g_service_array,service_name);
    if (service==NULL)
    {
        HD_LOGGER_WARNING(TAG,"[ipc_init_on_connected_internal]service:<%s> not fond!!!! \n",service_name);
        return ;
    }
    
    stpncpy(service->version, service_version, strlen(service_version));
    service->status = HD_SERVICE_STATUS_STARTED;
    service->update = 0;
    service->pid = service_pid;
    HD_LOGGER_ERROR(TAG, "[ipc_init_on_connected_internal] %s SERVICE-STARTED!!! %s %s\n", service_name, PREFIX, PREFIX);
    hd_service_array_print(&g_service_array);
    if (strcmp(HD_INIT_SERVICE_MAIN, service->name) == 0)
    {
        start_main_service_count = 0;
    }
    else if (strcmp(HD_INIT_SERVICE_RPC, service->name) == 0)
    {
        // 启动hdmqtt hduart
	 HDService *old_mqtt_service =  hd_service_array_find_by_name(&g_service_array,HD_INIT_SERVICE_MQTT);
	 if(old_mqtt_service == NULL){
		HDService mqtt_service = {
        	.name = HD_INIT_SERVICE_MQTT,
       		 .path = HD_INIT_SERVICE_MQTT_PATH,
        	.status = HD_SERVICE_STATUS_STOPPED,
        	.type = HD_SERVICE_TYPE_SECONDARY,
        	.depends_on_count = 1,
       		.version = "0.0.0",
        	.depends_on = {}};
    		hd_service_array_add(&g_service_array, &mqtt_service);
		op_start_service_internal(&mqtt_service);
	 } else {
	 	if(old_mqtt_service->status == HD_SERVICE_STATUS_STARTED){
		}else{
		  op_start_service_internal(old_mqtt_service);
		}
	 
	 }
	sleep(1);
	HDService *old_uart_service =  hd_service_array_find_by_name(&g_service_array,HD_INIT_SERVICE_UART);
	if(old_uart_service == NULL){
	 	HDService uart_service = {
        	.name = HD_INIT_SERVICE_UART,
        	.path = HD_INIT_SERVICE_UART_PATH,
        	.status = HD_SERVICE_STATUS_STOPPED,
        	.type = HD_SERVICE_TYPE_SECONDARY,
        	.depends_on_count = 1,
        	.version = "0.0.0",
        	.depends_on = {}};
    		hd_service_array_add(&g_service_array, &uart_service);
		op_start_service_internal(&uart_service);
	} else {
		if(old_uart_service->status == HD_SERVICE_STATUS_STARTED){
                }else{
                  op_start_service_internal(old_uart_service);
                }

	}

	sleep(1);
	 HDService *old_qt_service =  hd_service_array_find_by_name(&g_service_array,HD_INIT_SERVICE_QT);
        if(old_uart_service == NULL){
                HDService qt_service = {
                .name = HD_INIT_SERVICE_QT,
                .path = HD_INIT_SERVICE_QT_PATH,
                .status = HD_SERVICE_STATUS_STOPPED,
                .type = HD_SERVICE_TYPE_SECONDARY,
                .depends_on_count = 1,
                .version = "0.0.0",
                .depends_on = {}};
                hd_service_array_add(&g_service_array, &qt_service);
                op_start_service_internal(&qt_service);
        } else {
                if(old_qt_service->status == HD_SERVICE_STATUS_STARTED){
                }else{
                  op_start_service_internal(old_qt_service);
                }

        }


//	int ret = start_qt();
//	HD_LOGGER_ERROR(TAG,"======== > >>>>>>>>start_qt() fail!");
    }
    g_blocking = 0;
}

static void * hd_ipc_init_thread_function(void *arg){
    ipc_init_initialize(
        ipc_init_on_connected_internal,
        ipc_init_on_heartbeat_lost_internal,
        ipc_resp_cmd_ipc_core_heartbeat_pong,
        ipc_resp_cmd_ipc_shell_confirm_upgrade_resp
    );
    return NULL;
}

/**
 * 
 *   gcc -o ./.service/hd_init hd_init.c hd_logger.c hd_utils.c   hd_ipc_init.c hd_service.c hd_ipc.c -lpthread hd_http.c ./cJSON.c -L./libs/ -I./includes -lcurl -ljsonrpcc -lev -lhdsafemap hd_ipc_protocol.c
 * 
 *   cp .hdinit/bak/hdmain-0.0.3 ./.service/hdmain
 * 
 * 
 *  export LD_LIBRARY_PATH=./libs:$LD_LIBRARY_PATH          linux
 *  export DYLD_LIBRARY_PATH=./libs:$DYLD_LIBRARY_PATH   mac
 * 
 */
int main(int argc, char const *argv[])
{
    /*[注册信号]*/
    // signal(SIGINT, hd_init_sigint_handler);  // 注册信号处理器 ctrl + c
    // signal(SIGTSTP, hd_init_sigint_handler);  // 注册信号处理器 ctrl + z
    // signal(SIGUSR1, hd_init_sigint_handler);  //

    signal(SIGPIPE, SIG_IGN); // 当客户端（子进程）主动结束unix domain socket ,导致程序退出。

    // map_init(&g_map);

    // 开启ipc
    pthread_t hd_ipc_init_thread_t;
    pthread_create(&hd_ipc_init_thread_t,NULL,hd_ipc_init_thread_function,NULL);

    // struct sigaction sa_usr1;
    // sa_usr1.sa_sigaction = sig_heart_beat_handler;
    // sigemptyset(&sa_usr1.sa_mask);
    // sigaddset(&sa_usr1.sa_mask, SIGTERM); // 阻塞 SIGTERM
    // sa_usr1.sa_flags = 0;

    // sigaction(SIGUSR1, &sa_usr1, NULL);

    if (HD_DEBUG)
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    }
    else
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_INFO);
    }

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

        // exit(EXIT_FAILURE);
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
    HD_LOGGER_INFO(TAG, "[start_core_services size=%d... ]\n", g_service_array.count);
    ret = start_core_services();
    if (ret < 0)
    {
        HD_LOGGER_ERROR(TAG, "[start_core_services error! ]\n");
        hd_init_exit();
        // exit(EXIT_FAILURE);
    }

    /* [启动monitor线程] */
    HD_LOGGER_INFO(TAG, "[monitor_services_thread_t started !!!]\n");
    pthread_t monitor_services_thread_t;
    pthread_create(&monitor_services_thread_t, NULL, monitor_thread_services_thread_by_select, NULL);

    /* 启动upgrade线程 */
    HD_LOGGER_INFO(TAG, "[upgrade_services_thread_t started !!!] \n");
    pthread_t upgrade_services_thread_t;
    pthread_create(&upgrade_services_thread_t, NULL, upgrade_thread_services_thread_by_select, NULL);

    pthread_join(monitor_services_thread_t, NULL);
    HD_LOGGER_INFO(TAG, "[monitor_services_thread_t end !!!] \n");
    pthread_join(upgrade_services_thread_t, NULL);
    HD_LOGGER_INFO(TAG, "[upgrade_services_thread_t end !!!] \n");

    /* [hdinit运行结束] */
    HD_LOGGER_INFO(TAG, "[hdinit end!] \n");

    // printf("准备自杀...\n");
    // kill(getpid(), SIGKILL);  // SIGTERM 更温和
    // kill(getpid(), SIGTERM);  // SIGTERM 更温和
    // printf("这行不会执行\n");

    // map_free(&g_map);

    hd_service_array_cleanup(&g_service_array);
    ipc_init_destory();
    return 0;
}
