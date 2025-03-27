#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define VERSION "1.0.0"
#define SOCKET_PATH "/tmp/hdinit_socket_2"

void print_help() {
    printf("hdinit commands:\n");
    printf("  -version           Show version\n");
    printf("  -ls                List all services\n");
    printf("  -detail <service>  Show service details\n");
    printf("  -help              Show this help\n");
    printf("  -splash <service>  Show loading splash for service\n");
    printf("  start <service>    Start a service\n");
    printf("  stop <service>     Stop a service\n");
    printf("  check <service>    Check if service has updates\n");
    printf("  register <name> <path> Register a new service\n");
    printf("  unregister <name>  Unregister a service\n");
    printf("  shutdown           Shutdown hdinit\n");
}

int send_command_to_hdinit(int argc, char *argv[]) {
	printf("======hdshell===send_command_hdinit=argc=%d==\n",argc);
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[1024];
    
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
	printf("======hdshell====socket error===\n");
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect error");
	printf("======hdshell====connect error===\n");
        close(sock_fd);
        return -1;
    }
    
    // 构建命令字符串
    char cmd[1024] = {0};
    if(argc==0){
	       sprintf(cmd,"hdinit NAN");	
    }else{
    
    
   	 for (int i = 0; i < argc; i++) {
       		 strcat(cmd, argv[i]);
       		 if (i < argc - 1) strcat(cmd, " ");
   	 }
    }
    // 设置接收超时为5秒
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    // 发送命令
    if (write(sock_fd, cmd, strlen(cmd)) == -1) {
        perror("write error");
	printf("======hdshell===write error====\n");
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

int main(int argc, char *argv[]) {

	printf("======hdshell=======\n");
    // 检查是否以hdinit别名运行
    if (strstr(argv[0], "hdinit") != NULL) {
        // 如果以hdinit别名调用，转发命令给真正的hdinit
	printf("======hdshell==init=====\n");
        int ret =  send_command_to_hdinit(argc - 1, argv + 1);
	printf("======hdshell==init=result:%d====\n",ret);
	return ret;
    }
    
	printf("======hdshell==2=====\n");
    // 正常hdshell交互模式
    if (argc > 1) {

	printf("======hdshell===3====\n");
        return send_command_to_hdinit(argc - 1, argv + 1);
    }
    
    printf("HD System Shell\n");
    printf("Type 'hdinit -help' for command list\n");
    
    // 交互式循环
    char input[256];
    while (1) {
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
            if (strcmp(args[0], "help") == 0 || strcmp(args[0], "-help") == 0) {
                print_help();
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
    }
    
    return 0;
}
