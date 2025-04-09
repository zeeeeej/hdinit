#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "hd_logger.h"
#include "hd_ipc.h"
#include "hd_logger.h"
#include "hd_utils.h"
#include <fcntl.h>
#include <pthread.h>
#include "cJSON.h"


#define TAG "hdshell"

volatile sig_atomic_t running = 1;

struct recv_thread_func_data
{
    int  sock_fd;
} ;

int send_command_to_hdinit(int argc,  const char *argv[]);

/**
 * 
    {
        "cmd"   : "print",
        "param" : 
        {
            "msg"   : "Service [hdma] not found\n"
        }
    }
 */
static int parse_cmd(int sock_fd, const char  *buffer,size_t size){
    char  cmd[128] = {0};
    // 解析 JSON 字符串
    // hd_print_buffer(buffer,256);
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            // fprintf(stderr, "JSON parse error>>>: %s\n", error_ptr);
        }
        return -1;
    }
    // 提取 "cmd" 字段
    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (cmd_item == NULL || !cJSON_IsString(cmd_item)) {
        fprintf(stderr, "Failed to parse 'cmd' field\n");
        cJSON_Delete(root);
        return -1;
    }
    strncpy(cmd, cmd_item->valuestring, strlen(cmd_item->valuestring) + 1);

    if(strcmp("print",cmd) == 0)
    {


        char  msg[1024] = {0};
         // 提取嵌套的 "param.msg" 字段
        cJSON *param_item = cJSON_GetObjectItem(root, "param");
        if (param_item == NULL || !cJSON_IsObject(param_item)) {
            fprintf(stderr, "Failed to parse 'param' object\n");
            cJSON_Delete(root);
            return -1;
        }

        cJSON *msg_item = cJSON_GetObjectItem(param_item, "msg");
        if (msg_item == NULL || !cJSON_IsString(msg_item)) {
            fprintf(stderr, "Failed to parse 'msg' field\n");
            cJSON_Delete(root);
            return -1;
        }
        strncpy(msg, msg_item->valuestring, strlen(msg_item->valuestring) + 1);

        printf("> %s \n",msg);

    } 
    else if(strcmp("error",cmd) == 0)
    {
        
        char  msg[1024] = {0};
         // 提取嵌套的 "param.msg" 字段
        cJSON *param_item = cJSON_GetObjectItem(root, "param");
        if (param_item == NULL || !cJSON_IsObject(param_item)) {
            fprintf(stderr, "Failed to parse 'param' object\n");
            cJSON_Delete(root);
            return -1;
        }

        cJSON *error_item = cJSON_GetObjectItem(param_item, "error");
        if (error_item == NULL || !cJSON_IsString(error_item)) {
            fprintf(stderr, "Failed to parse 'msg' field\n");
            cJSON_Delete(root);
            return -1;
        }
        strncpy(msg, error_item->valuestring, strlen(error_item->valuestring) + 1);

        cJSON *error_code_item = cJSON_GetObjectItem(param_item, "code");
        if (error_code_item == NULL || !cJSON_IsNumber(error_code_item)) {
            fprintf(stderr, "Failed to parse 'msg' field\n");
            cJSON_Delete(root);
            return -1;
        }
        int code = error_code_item->valueint;
        printf("> %s (%d)\n",msg,code);
    } 
    else if(strcmp("check_result",cmd) == 0)
    {
        // {
        //     "cmd": "check_result",
        //     "param": {
        //       "url": "http://10.13.13.114:5000/files/hdmain-0.0.4",
        //       "md5": "ae1d2c8b2847941e6572479815637e7f",
        //       "version": "0.0.4",
        //       "service": "hdmain",
        //       "filename": "hdmain-0.0.4"
        //     }
        //   }
        
        char input;
        input = getchar();  // 读取一个字符
            if (input == 'y' || input == 'Y') {
                input = 'y';
            }else{
                input = 'n';
            }
        sprintf(cmd,"%c",input);	
        write(sock_fd, cmd, strlen(cmd) );

    }
    else
    {
        HD_LOGGER_DEBUG(TAG,"parse_cmd not support %s \n",cmd);
        return 0;
    }
    return 0;
}

void * recv_thread_func(void * arg){
    struct recv_thread_func_data *data = (struct recv_thread_func_data*)arg;
    // 接收响应
    char buffer[HD_IPC_SOCKET_PATH_BUFF_SIZE] = {0};
    int n;
    int ret ;
    while ((n = read(data->sock_fd, buffer, sizeof(buffer) - 1)) > 0) {
        // HD_LOGGER_DEBUG(TAG,"%s\n",buffer);
        // 解析回复
        buffer[strlen(buffer)] = '\0';
        ret =  parse_cmd(data->sock_fd,buffer,strlen(buffer));
        
        if (ret!=0)
        {
                printf("$%s \n",buffer);
        }
        // reset
        memset(buffer,0,sizeof(buffer));
    }
    return NULL;
}

/** start */

int send_command_to_hdinit(int argc,  const char *argv[]) { 
	HD_LOGGER_DEBUG(TAG,"send_command_to_hdinit (%d).\n",argc);
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[1024];
    
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        HD_LOGGER_ERROR(TAG,"send_command_to_hdinit socket error!\n");
        return -1;
    }
    
    // 确保路径可访问
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, HD_IPC_SOCKET_PATH);

    //fcntl(sock_fd, F_SETFL, O_NONBLOCK); // 非阻塞模式
    
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        HD_LOGGER_ERROR(TAG,"send_command_to_hdinit socket connect [%s]error : %s(errno=%d)!\n",HD_IPC_SOCKET_PATH,strerror(errno), errno);
        close(sock_fd);
        return -1;
    }

    //HD_LOGGER_ERROR(TAG,"\n\nsend_command_to_hdinit  recv_thread_func ! \n\n");
    pthread_t read_t;
    struct recv_thread_func_data data = {
        .sock_fd = sock_fd
    };
    pthread_create(&read_t,NULL,recv_thread_func,&data);
    
    // 构建命令字符串
    char cmd[1024] = {0};
    if(argc==0){
	    sprintf(cmd," -h");	
    }else{
        for (int i = 0; i < argc; i++) {
            if (strcmp(DEBUG_PARAM,argv[i])==0){
                continue;
            }
            strcat(cmd, argv[i]);           
            if (i < argc - 1) strcat(cmd, " ");
        }
    }
    if (strcmp("",cmd)==0)
    {
        sprintf(cmd," -h");	
    }
    HD_LOGGER_DEBUG(TAG,"send_command_to_hdinit cmd:<%s>!\n",cmd);
  
    // 设置接收超时为5秒
    // struct timeval tv;
    // tv.tv_sec = 5;
    // tv.tv_usec = 0; 
    // setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // 发送命令
    if (write(sock_fd, cmd, strlen(cmd)) == -1) {
	    HD_LOGGER_ERROR(TAG,"send_command_to_hdinit  socket write error!\n");
        close(sock_fd);
        return -1;
    }
    pthread_join(read_t,NULL);
    close(sock_fd);
    return 0;
}
/** end */

void handle_signal(int sig) {
    HD_LOGGER_INFO("hdshell",">>> Shell service handle_signal (PID: %d) sig=%d\n", getpid(),sig);
    running = 0;
}

/**
 * gcc hd_shell.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -lpthread -o hdshell
 */
int main(int argc,  const char *argv[]) {
    if (HD_DEBUG)
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    }
    else
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_INFO); 
    }
    int debug = hd_check_argc_argv_has_debug(argc,argv); 
    if (debug == 0){
        hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    }else{
        hd_logger_set_level(HD_LOGGER_LEVEL_INFO);
    }
    
    HD_LOGGER_DEBUG(TAG,">>> HDShell service started (PID: %d)\n", getpid());
    //int argc;
    //char **argv;
    //hd_argc_argv_remove_debug(_argc,_argv,&argc,argv);

    // hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);

    signal(SIGTERM, handle_signal);

    // if (argc==2 && strcmp(argv[1], "-h")==0  && (strstr(argv[0], "hdinit")!=NULL || strstr(argv[0], "hdshell")  != NULL)) {
    //     ipc_print_help();
    //     return 0;
    // }

    //if (argc>0 && strstr(argv[0], "hdinit") != NULL) {
        HD_LOGGER_DEBUG(TAG," <hdinit mode> \n");
        // 如果以hdinit别名调用，转发命令给真正的hdinit
          //char * params[] = argv + 1;
        int ret =  send_command_to_hdinit(argc - 1, argv+1);
        HD_LOGGER_DEBUG(TAG,"<<< send_command_to_hdinit result:%d \n", ret);
        if (ret!=0)
        {
            //ipc_print_help();
        }
        

    //}

    /*
    // 正常hdshell交互模式
    if (argc > 1) {
        HD_LOGGER_INFO(TAG," <hdshell mode> \n");
        int ret =  send_command_to_hdinit(argc - 1, argv + 1);
        if (ret!=0)
        {
            ipc_print_help();
        }
        return ret;
    }
        
    HD_LOGGER_INFO(TAG," <circle mode> \n");
    printf("HD System Shell\n");
    printf("Type 'hdinit -help' for command list\n");
        
    // 交互式循环
    char input[256];
    while (running) {
            printf("hdshell> ");
            if (fgets(input, sizeof(input), stdin) == NULL) break;
            
            // 解析输入
            char *args[10];
            int arg_count = 0;
            char *token = strtok(input, " \n");
            
            while (token != NULL && arg_count < 10) {
                args[arg_count++] = token;
                token = strtok(NULL, " \n");
            }
            
            if (arg_count > 0) {
                // 特殊处理help命令
                if (strcmp(args[0], "h") == 0 || strcmp(args[0], "-h") == 0) {
                    ipc_print_help();
                    continue;
                }
                
                // 添加程序名作为第一个参数
                char *new_argv[arg_count + 1];
                new_argv[0] = "hdinit";
                for (int i = 0; i < arg_count; i++) {
                    new_argv[i + 1] = args[i];
                }
                
                send_command_to_hdinit(arg_count + 1, new_argv);
            }
        }*/

    // HD_LOGGER_INFO(TAG,"<<< shell end ！！！！ \n", ret);
    return 0;
}