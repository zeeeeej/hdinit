#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

#define CONFIG_FILE "/etc/hdmain.conf"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

// 业务逻辑函数指针类型
typedef void (*business_logic_t)();

int load_business_logic(const char *plugin_path, business_logic_t *logic_func) {
    void *handle = dlopen(plugin_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading plugin: %s\n", dlerror());
        return -1;
    }

    // 清除可能的错误
    dlerror();

    *logic_func = (business_logic_t)dlsym(handle, "business_logic");
    const char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error finding symbol: %s\n", error);
        dlclose(handle);
        return -1;
    }

    return 0;
}

char* read_config(const char *config_file) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        perror("Error opening config file");
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // 读取第一行作为插件路径
    if ((read = getline(&line, &len, file)) == -1) {
        fclose(file);
        free(line);
        return NULL;
    }

    fclose(file);

    // 去除换行符
    if (line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    return line;
}

int main() {
    printf("hdmain service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);

    // 读取配置文件获取插件路径
    char *plugin_path = read_config(CONFIG_FILE);
    if (!plugin_path) {
        fprintf(stderr, "Using default built-in business logic\n");
        
        // 默认业务逻辑
        while (running) {
            sleep(10);
            printf("hdmain is running with default logic...\n");
        }
    } else {
        printf("Loading business logic from: %s\n", plugin_path);
        
        business_logic_t business_logic = NULL;
        if (load_business_logic(plugin_path, &business_logic) == 0) {
            // 使用插件中的业务逻辑
            while (running) {
                business_logic();
            }
        } else {
            fprintf(stderr, "Falling back to default business logic\n");
            
            // 回退到默认业务逻辑
            while (running) {
                sleep(10);
                printf("hdmain is running with fallback logic...\n");
            }
        }
        
        free(plugin_path);
    }
    
    printf("hdmain service stopped\n");
    return 0;
}
