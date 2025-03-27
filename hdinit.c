#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define MAX_SERVICES 20
#define VERSION "1.0.0"
#define SOCKET_PATH "/tmp/hdinit_socket_2"

typedef struct {
    char name[50];
    char path[256];
    pid_t pid;
    int is_running;
    time_t last_modified;
    int is_main;
    int is_secondary;
    int is_registered;
    int depends_on_count;
    char depends_on[5][50];
} Service;

Service services[MAX_SERVICES];
int service_count = 0;
int server_running = 1;

void init_services() {
    // 主服务
    strcpy(services[0].name, "hdmain");
    strcpy(services[0].path, "/root/hdmain");
    services[0].is_main = 1;
    services[0].is_secondary = 0;
    services[0].is_registered = 1;
    services[0].depends_on_count = 0;
    
    // 辅助服务
    strcpy(services[1].name, "hddomain");
    strcpy(services[1].path, "/root/hddomain");
    services[1].is_main = 0;
    services[1].is_secondary = 1;
    services[1].is_registered = 1;
    services[1].depends_on_count = 1;
    strcpy(services[1].depends_on[0], "hdmain");
    
    strcpy(services[2].name, "hdlog");
    strcpy(services[2].path, "/root/hdlog");
    services[2].is_main = 0;
    services[2].is_secondary = 1;
    services[2].is_registered = 1;
    services[2].depends_on_count = 1;
    strcpy(services[2].depends_on[0], "hdmain");
    
    strcpy(services[3].name, "hdsplash");
    strcpy(services[3].path, "/root/hdsplash");
    services[3].is_main = 0;
    services[3].is_secondary = 1;
    services[3].is_registered = 1;
    services[3].depends_on_count = 1;
    strcpy(services[3].depends_on[0], "hdmain");
    
    strcpy(services[4].name, "hdshell");
    strcpy(services[4].path, "/root/hdshell");
    services[4].is_main = 0;
    services[4].is_secondary = 1;
    services[4].is_registered = 1;
    services[4].depends_on_count = 2;
    strcpy(services[4].depends_on[0], "hdmain");
    strcpy(services[4].depends_on[1], "hdlog");
    
    service_count = 5;
}

time_t get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}

int start_service(int index) {
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
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        execl(services[index].path, services[index].name, NULL);
        exit(EXIT_FAILURE);
    } else {
        services[index].pid = pid;
        services[index].is_running = 1;
        services[index].last_modified = get_last_modified(services[index].path);
        return 0;
    }
}

int stop_service(int index) {
    if (services[index].is_running) {
        if (kill(services[index].pid, SIGTERM) == 0) {
		sleep(1); // 给子进程时间清理
		int child_pid = services[index].pid;
		int status;
// 检查子进程是否还在运行
if (waitpid(child_pid, &status, WNOHANG) == 0) {
    // 如果还在运行，强制终止
    kill(child_pid, SIGKILL);
    waitpid(child_pid, &status, 0);
}
            services[index].is_running = 0;
            return 0;
        } else {
            perror("kill failed");
            return -1;
        }
    }
    return 0;
}

int check_service_update(int index) {
    time_t current_mtime = get_last_modified(services[index].path);
    return current_mtime > services[index].last_modified;
}

int register_service(const char *name, const char *path) {
    if (service_count >= MAX_SERVICES) {
        return -1;
    }
    
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return -1;
        }
    }
    
    strcpy(services[service_count].name, name);
    strcpy(services[service_count].path, path);
    services[service_count].is_main = 0;
    services[service_count].is_secondary = 0;
    services[service_count].is_registered = 1;
    services[service_count].depends_on_count = 0;
    services[service_count].is_running = 0;
    services[service_count].last_modified = get_last_modified(path);
    
    service_count++;
    return 0;
}

int unregister_service(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            if (services[i].is_running) {
                stop_service(i);
            }
            
            if (services[i].is_main || services[i].is_secondary) {
                return -1;
            }
            
            for (int j = i; j < service_count - 1; j++) {
                services[j] = services[j + 1];
            }
            
            service_count--;
            return 0;
        }
    }
    
    return -1;
}

void monitor_services() {
    for (int i = 0; i < service_count; i++) {
        if (services[i].is_running) {
            if (kill(services[i].pid, 0) != 0) {
                services[i].is_running = 0;
                start_service(i);
            }
            
            if (check_service_update(i)) {
                stop_service(i);
                start_service(i);
            }
        }
    }
}

int find_service_index(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void list_services(int client_fd) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%-15s %-10s %-10s %s\n", "NAME", "STATUS", "TYPE", "PATH");
    write(client_fd, buffer, strlen(buffer));
    
    for (int i = 0; i < service_count; i++) {
        snprintf(buffer, sizeof(buffer), "%-15s %-10s %-10s %s\n", 
               services[i].name, 
               services[i].is_running ? "RUNNING" : "STOPPED",
               services[i].is_main ? "MAIN" : 
                 (services[i].is_secondary ? "SECONDARY" : "OTHER"),
               services[i].path);
        write(client_fd, buffer, strlen(buffer));
    }
}

void show_service_detail(int client_fd, const char *name) {
    int idx = find_service_index(name);
    char buffer[1024];
    
    if (idx == -1) {
        snprintf(buffer, sizeof(buffer), "Service %s not found\n", name);
        write(client_fd, buffer, strlen(buffer));
        return;
    }
    
    snprintf(buffer, sizeof(buffer), "Service: %s\n", services[idx].name);
    write(client_fd, buffer, strlen(buffer));
    
    snprintf(buffer, sizeof(buffer), "Path: %s\n", services[idx].path);
    write(client_fd, buffer, strlen(buffer));
    
    snprintf(buffer, sizeof(buffer), "Type: %s\n", services[idx].is_main ? "Main" : 
                            (services[idx].is_secondary ? "Secondary" : "Other"));
    write(client_fd, buffer, strlen(buffer));
    
    snprintf(buffer, sizeof(buffer), "Status: %s\n", services[idx].is_running ? "Running" : "Stopped");
    write(client_fd, buffer, strlen(buffer));
    
    if (services[idx].is_running) {
        snprintf(buffer, sizeof(buffer), "PID: %d\n", services[idx].pid);
        write(client_fd, buffer, strlen(buffer));
    }
    
    snprintf(buffer, sizeof(buffer), "Last modified: %s", ctime(&services[idx].last_modified));
    write(client_fd, buffer, strlen(buffer));
    
    snprintf(buffer, sizeof(buffer), "Dependencies: ");
    write(client_fd, buffer, strlen(buffer));
    
    if (services[idx].depends_on_count == 0) {
        snprintf(buffer, sizeof(buffer), "None\n");
    } else {
        for (int i = 0; i < services[idx].depends_on_count; i++) {
            snprintf(buffer, sizeof(buffer), "%s ", services[idx].depends_on[i]);
            write(client_fd, buffer, strlen(buffer));
        }
        snprintf(buffer, sizeof(buffer), "\n");
    }
    write(client_fd, buffer, strlen(buffer));
}

void handle_client_command(int client_fd, int argc, char *argv[]) {
    char buffer[1024];
    
	printf("======hdinit==handle_client_command=====\n");
		

    if (argc < 1) {
        snprintf(buffer, sizeof(buffer), "Usage: hdinit <command> [options]\n");
        write(client_fd, buffer, strlen(buffer));
        return;
    }
    
	printf("======hdinit==argv[0]%s=====\n",argv[0]);
    if (strcmp(argv[0], "-version") == 0) {
        snprintf(buffer, sizeof(buffer), "hdinit version %s\n", VERSION);
        write(client_fd, buffer, strlen(buffer));
    } 
    else if (strcmp(argv[0], "-ls") == 0) {
        list_services(client_fd);
    }
    else if (strcmp(argv[0], "-detail") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit -detail <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        show_service_detail(client_fd, argv[1]);
    }
    else if (strcmp(argv[0], "-help") == 0) {
        snprintf(buffer, sizeof(buffer), "hdinit commands:\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  -version           Show version\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  -ls                List all services\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  -detail <service>  Show service details\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  -help              Show this help\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  -splash <service>  Show loading splash for service\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  start <service>    Start a service\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  stop <service>     Stop a service\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  check <service>    Check if service has updates\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  register <name> <path> Register a new service\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  unregister <name>  Unregister a service\n");
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "  shutdown           Shutdown hdinit\n");
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "-splash") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit -splash <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        snprintf(buffer, sizeof(buffer), "Showing loading splash for service %s...\n", argv[1]);
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "start") == 0) {
        if (argc < 2) {
            snprintf(buffer, sizeof(buffer), "Usage: hdinit start <service name>\n");
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        int idx = find_service_index(argv[1]);
        if (idx == -1) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (services[idx].is_running) {
            snprintf(buffer, sizeof(buffer), "Service %s is already running\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (start_service(idx) == 0) {
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
        int idx = find_service_index(argv[1]);
        if (idx == -1) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (!services[idx].is_running) {
            snprintf(buffer, sizeof(buffer), "Service %s is not running\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (stop_service(idx) == 0) {
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
        int idx = find_service_index(argv[1]);
        if (idx == -1) {
            snprintf(buffer, sizeof(buffer), "Service %s not found\n", argv[1]);
            write(client_fd, buffer, strlen(buffer));
            return;
        }
        if (check_service_update(idx)) {
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
        if (register_service(argv[1], argv[2]) == 0) {
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
        if (unregister_service(argv[1]) == 0) {
            snprintf(buffer, sizeof(buffer), "Unregistered service %s\n", argv[1]);
        } else {
            snprintf(buffer, sizeof(buffer), "Failed to unregister service %s\n", argv[1]);
        }
        write(client_fd, buffer, strlen(buffer));
    }
    else if (strcmp(argv[0], "shutdown") == 0) {
        snprintf(buffer, sizeof(buffer), "Shutting down all services...\n");
        write(client_fd, buffer, strlen(buffer));
        for (int i = 0; i < service_count; i++) {
            if (services[i].is_running) {
                stop_service(i);
            }
        }
        server_running = 0;
        snprintf(buffer, sizeof(buffer), "Goodbye!\n");
        write(client_fd, buffer, strlen(buffer));
    }
    else {
        snprintf(buffer, sizeof(buffer), "Unknown command: %s\n", argv[0]);
        write(client_fd, buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "Use 'hdinit -help' for usage information\n");
        write(client_fd, buffer, strlen(buffer));
    }
}

void *ipc_server_thread(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    unlink(SOCKET_PATH);
    
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    
    while (server_running) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            if (server_running) {
                perror("accept error");
            }
            continue;
        }
        
        char cmd[1024];
        int n = read(client_fd, cmd, sizeof(cmd) - 1);
        if (n > 0) {
            cmd[n] = '\0';
            
            // 解析命令
            char *argv[10];
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
    unlink(SOCKET_PATH);
    return NULL;
}

int main() {
    printf("Starting hdinit service manager (version %s)...\n", VERSION);
    
    init_services();
    
    // 启动IPC服务器线程
    pthread_t ipc_thread;
    if (pthread_create(&ipc_thread, NULL, ipc_server_thread, NULL) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    // 启动主服务
    start_service(0); // hdmain
    
    sleep(2);
    
    // 启动辅助服务
    for (int i = 1; i < service_count; i++) {
        start_service(i);
    }
    
    // 主监控循环
    while (server_running) {
        monitor_services();
        sleep(5);
    }
    
    pthread_join(ipc_thread, NULL);
    return 0;
}
