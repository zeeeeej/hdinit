#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_SERVICES 20
#define VERSION "1.0.0"

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
    char depends_on[5][50]; // 最多依赖5个其他服务
} Service;

Service services[MAX_SERVICES];
int service_count = 0;

// 初始化内置服务
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

// 检查文件最后修改时间
time_t get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}

// 启动服务
int start_service(int index) {
    // 检查依赖服务是否都运行
    for (int i = 0; i < services[index].depends_on_count; i++) {
        int found = 0;
        for (int j = 0; j < service_count; j++) {
            if (strcmp(services[j].name, services[index].depends_on[i]) == 0) {
                found = 1;
                if (!services[j].is_running) {
                    printf("Dependency service %s is not running\n", services[j].name);
                    return -1;
                }
                break;
            }
        }
        if (!found) {
            printf("Dependency service %s not found\n", services[index].depends_on[i]);
            return -1;
        }
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        // 子进程
        execl(services[index].path, services[index].name, NULL);
        exit(EXIT_FAILURE);
    } else {
        // 父进程
        services[index].pid = pid;
        services[index].is_running = 1;
        services[index].last_modified = get_last_modified(services[index].path);
        printf("Started service %s (PID: %d)\n", services[index].name, pid);
        return 0;
    }
}

// 停止服务
int stop_service(int index) {
    if (services[index].is_running) {
        if (kill(services[index].pid, SIGTERM) == 0) {
            services[index].is_running = 0;
            printf("Stopped service %s (PID: %d)\n", services[index].name, services[index].pid);
            return 0;
        } else {
            perror("kill failed");
            return -1;
        }
    }
    return 0;
}

// 检查服务是否更新
int check_service_update(int index) {
    time_t current_mtime = get_last_modified(services[index].path);
    if (current_mtime > services[index].last_modified) {
        printf("Service %s has been updated (old: %ld, new: %ld)\n", 
               services[index].name, services[index].last_modified, current_mtime);
        return 1;
    }
    return 0;
}

// 注册新服务
int register_service(const char *name, const char *path) {
    if (service_count >= MAX_SERVICES) {
        printf("Maximum number of services reached\n");
        return -1;
    }
    
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            printf("Service %s already exists\n", name);
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
    printf("Registered service %s at %s\n", name, path);
    return 0;
}

// 注销服务
int unregister_service(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            if (services[i].is_running) {
                stop_service(i);
            }
            
            // 如果是内置服务，不能注销
            if (services[i].is_main || services[i].is_secondary) {
                printf("Cannot unregister built-in service %s\n", name);
                return -1;
            }
            
            // 移动数组元素
            for (int j = i; j < service_count - 1; j++) {
                services[j] = services[j + 1];
            }
            
            service_count--;
            printf("Unregistered service %s\n", name);
            return 0;
        }
    }
    
    printf("Service %s not found\n", name);
    return -1;
}

// 监控所有服务
void monitor_services() {
    for (int i = 0; i < service_count; i++) {
        if (services[i].is_running) {
            // 检查进程是否还在运行
            if (kill(services[i].pid, 0) != 0) {
                printf("Service %s (PID: %d) has stopped unexpectedly, restarting...\n", 
                       services[i].name, services[i].pid);
                services[i].is_running = 0;
                start_service(i);
            }
            
            // 检查是否有更新
            if (check_service_update(i)) {
                printf("Restarting service %s due to update...\n", services[i].name);
                stop_service(i);
                start_service(i);
            }
        }
    }
}

// 查找服务索引
int find_service_index(const char *name) {
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 处理shell命令
void handle_command(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: hdinit <command> [options]\n");
        return;
    }
    
    if (strcmp(argv[1], "-version") == 0) {
        printf("hdinit version %s\n", VERSION);
    } 
    else if (strcmp(argv[1], "-ls") == 0) {
        printf("%-15s %-10s %-10s %s\n", "NAME", "STATUS", "TYPE", "PATH");
        for (int i = 0; i < service_count; i++) {
            printf("%-15s %-10s %-10s %s\n", 
                   services[i].name, 
                   services[i].is_running ? "RUNNING" : "STOPPED",
                   services[i].is_main ? "MAIN" : 
                     (services[i].is_secondary ? "SECONDARY" : "OTHER"),
                   services[i].path);
        }
    }
    else if (strcmp(argv[1], "-detail") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit -detail <service name>\n");
            return;
        }
        int idx = find_service_index(argv[2]);
        if (idx == -1) {
            printf("Service %s not found\n", argv[2]);
            return;
        }
        
        printf("Service: %s\n", services[idx].name);
        printf("Path: %s\n", services[idx].path);
        printf("Type: %s\n", services[idx].is_main ? "Main" : 
                            (services[idx].is_secondary ? "Secondary" : "Other"));
        printf("Status: %s\n", services[idx].is_running ? "Running" : "Stopped");
        if (services[idx].is_running) {
            printf("PID: %d\n", services[idx].pid);
        }
        printf("Last modified: %s", ctime(&services[idx].last_modified));
        printf("Dependencies: ");
        if (services[idx].depends_on_count == 0) {
            printf("None\n");
        } else {
            for (int i = 0; i < services[idx].depends_on_count; i++) {
                printf("%s ", services[idx].depends_on[i]);
            }
            printf("\n");
        }
    }
    else if (strcmp(argv[1], "-help") == 0) {
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
    else if (strcmp(argv[1], "-splash") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit -splash <service name>\n");
            return;
        }
        printf("Showing loading splash for service %s...\n", argv[2]);
        // 实际实现会调用hdsplash服务来显示加载界面
    }
    else if (strcmp(argv[1], "start") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit start <service name>\n");
            return;
        }
        int idx = find_service_index(argv[2]);
        if (idx == -1) {
            printf("Service %s not found\n", argv[2]);
            return;
        }
        if (services[idx].is_running) {
            printf("Service %s is already running\n", argv[2]);
            return;
        }
        start_service(idx);
    }
    else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit stop <service name>\n");
            return;
        }
        int idx = find_service_index(argv[2]);
        if (idx == -1) {
            printf("Service %s not found\n", argv[2]);
            return;
        }
        if (!services[idx].is_running) {
            printf("Service %s is not running\n", argv[2]);
            return;
        }
        stop_service(idx);
    }
    else if (strcmp(argv[1], "check") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit check <service name>\n");
            return;
        }
        int idx = find_service_index(argv[2]);
        if (idx == -1) {
            printf("Service %s not found\n", argv[2]);
            return;
        }
        if (check_service_update(idx)) {
            printf("Service %s has updates\n", argv[2]);
        } else {
            printf("Service %s is up to date\n", argv[2]);
        }
    }
    else if (strcmp(argv[1], "register") == 0) {
        if (argc < 4) {
            printf("Usage: hdinit register <service name> <path>\n");
            return;
        }
        register_service(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "unregister") == 0) {
        if (argc < 3) {
            printf("Usage: hdinit unregister <service name>\n");
            return;
        }
        unregister_service(argv[2]);
    }
    else if (strcmp(argv[1], "shutdown") == 0) {
        printf("Shutting down all services...\n");
        for (int i = 0; i < service_count; i++) {
            if (services[i].is_running) {
                stop_service(i);
            }
        }
        printf("Goodbye!\n");
        exit(0);
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        printf("Use 'hdinit -help' for usage information\n");
    }
}

int main(int argc, char *argv[]) {
    // 如果是shell命令调用
    if (argc > 1 && strstr(argv[0], "hdshell") != NULL) {
        handle_command(argc, argv);
        return 0;
    }
    
    printf("Starting hdinit service manager (version %s)...\n", VERSION);
    
    // 初始化服务列表
    init_services();
    
    // 启动主服务
    start_service(0); // hdmain
    
    // 等待主服务启动完成
    sleep(2);
    
    // 启动辅助服务
    for (int i = 1; i < service_count; i++) {
        start_service(i);
    }
    
    // 主监控循环
    while (1) {
        monitor_services();
        sleep(5); // 每5秒检查一次
    }
    
    return 0;
}
