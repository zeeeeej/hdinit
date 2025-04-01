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

#define TAG "hdshell"


volatile sig_atomic_t running = 1;

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
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, HD_IPC_SOCKET_PATH);
    
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        HD_LOGGER_ERROR(TAG,"send_command_to_hdinit socket connect [%s]error : %s(errno=%d)!\n",HD_IPC_SOCKET_PATH,strerror(errno), errno);
        close(sock_fd);
        return -1;
    }
    
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
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0; 
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    // 发送命令
    if (write(sock_fd, cmd, strlen(cmd)) == -1) {
	    HD_LOGGER_ERROR(TAG,"send_command_to_hdinit  socket write error!\n");
        close(sock_fd);
        return -1;
    }
    
    // 接收响应
    int n;
    while ((n = read(sock_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    
    close(sock_fd);
    return 0;
}
/** end */

void handle_signal(int sig) {
    HD_LOGGER_INFO("hdshell",">>> Shell service handle_signal (PID: %d) sig=%d\n", getpid(),sig);
    running = 0;
}

/**
 * gcc hd_shell.c hd_logger.c -o hdshell
 */
int main(int argc,  const char *argv[]) {
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
        
        return ret;
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
    return 0;
}