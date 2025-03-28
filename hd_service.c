#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "hd_service.h"

#define PRINT_PRETTY 1


// 初始化服务数组
void hd_service_array_init(HDServiceArray *sa) {
    if (sa == NULL) return;
    sa->count = 0;
    memset(sa->services, 0, sizeof(sa->services));
}

// 打印单个服务信息
void hd_service_print(const HDService *svc) {
    if (svc == NULL) {
        printf("Service: (null)\n");
        return;
    }

    printf("Service: %s\n", svc->name);
    printf("  Path: %s\n", svc->path);
    printf("  Version: %s\n", svc->version);
    printf("  PID: %d\n", svc->pid);
    printf("  Running: %s\n", svc->is_running ? "Yes" : "No");
    printf("  Main: %s\n", svc->is_main ? "Yes" : "No");
    printf("  Secondary: %s\n", svc->is_secondary ? "Yes" : "No");
    printf("  Registered: %s\n", svc->is_registered ? "Yes" : "No");
    
    char time_buf[26];
    ctime_r(&svc->last_modified, time_buf);
    time_buf[24] = '\0'; // Remove newline
    printf("  Last Modified: %s\n", time_buf);
    
    printf("  Dependencies (%d):\n", svc->depends_on_count);
    for (int i = 0; i < svc->depends_on_count; i++) {
        printf("    - %s\n", svc->depends_on[i]);
    }
}

static void hd_service_array_print_pretty(const HDServiceArray *sa, char * const result) ;
// 打印所有服务信息
void hd_service_array_print(const HDServiceArray *sa) {
    char result[2048];
    if (PRINT_PRETTY){
        hd_service_array_print_pretty(sa,result);
        printf("%s",result);
        return;
    }
    if (sa == NULL) {
        printf("Service array is null\n");
        return;
    }
    printf("===========hd_service_array_print===============\n");

    printf("Total services: %d\n", sa->count);
    for (int i = 0; i < sa->count; i++) {
        hd_service_print(&sa->services[i]);
        printf("\n");
    }
}

static void hd_service_array_print_pretty(const HDServiceArray *sa, char * const result) {
    if (sa == NULL || result == NULL) {
        if (result) strcpy(result, "Service array is null\n");
        return;
    }

    char buffer[2048];
    char temp[2048];
    result[0] = '\0'; // 清空结果字符串

    // 添加标题行
    snprintf(temp, sizeof(temp), "\n-------HDServiceArray[%d]-------\n", sa->count);
    strcat(result, temp);

    snprintf(temp, sizeof(temp), "%-10s %-10s %-10s %-10s %-10s %s\n", 
            "[NAME]", "[PID]", "[STATUS]", "[VERSION]", "[TYPE]", "[PATH]");
    strcat(result, temp);

    // 添加每个服务的信息
    const HDService *service;
    for (int i = 0; i < sa->count; i++) {
        service = &(sa->services[i]);
        snprintf(buffer, sizeof(buffer), 
                "%-10s %-10d %-10s %-10s %-10s %s %p\n", 
                service->name, 
                service->pid, 
                service->is_running ? "RUNNING" : "STOPPED",
                service->version == NULL ? "-" : service->version,
                service->is_main ? "MAIN" : (service->is_secondary ? "SECONDARY" : "OTHER"),
                service->path,
                service);
        strcat(result, buffer);
    }

    strcat(result, "\n"); // 添加结尾换行
}

static void hd_service_array_print_pretty_old(const HDServiceArray *sa) {
    if (sa == NULL) {
        printf("Service array is null\n");
        return;
    }
    printf("\n");
    printf("-------HDServiceArray[%d]-------\n",sa->count);
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "%-10s %-10s %-10s %-10s %-10s %s\n", "[NAME]","[PID]", "[STATUS]", "[VERSION]","[TYPE]", "[PATH]");
    printf("%s",buffer);
    const HDService *service;
    for (int i = 0; i < sa->count; i++) {
        service = &(sa->services[i]);
        snprintf(
            buffer, 
            sizeof(buffer), 
            "%-10s %-10d %-10s %-10s %-10s %s %p\n", 
            service->name, 
            service->pid, 
            service->is_running ? "RUNNING" : "STOPPED",
            service->version==NULL?"-":service->version ,
            service->is_main ? "MAIN" : (service->is_secondary ? "SECONDARY" : "OTHER"),
            service->path,
            service
            );
            printf("%s",buffer);
          
    }
    printf("\n");
 

}

// 根据名称查找服务索引 (-1表示未找到)
int hd_service_array_find_index_by_name(const HDServiceArray *sa, const char *name) {
  
    
    if (sa == NULL || name == NULL) return -1;

    for (int i = 0; i < sa->count; i++) {
        if (strcmp(sa->services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 根据名称查找服务指针 (NULL表示未找到)
HDService* hd_service_array_find_by_name(HDServiceArray *sa, const char *name) {
    int index = hd_service_array_find_index_by_name(sa, name);
    return (index == -1) ? NULL : &sa->services[index];
}

// 根据索引获取服务指针 (NULL表示无效索引)
HDService* hd_service_array_get_by_index(HDServiceArray *sa, int index) {
    if (sa == NULL || index < 0 || index >= sa->count) {
        return NULL;
    }
    return &sa->services[index];
}

// 添加服务 (返回0成功，-1失败)
int hd_service_array_add(HDServiceArray *sa, const HDService *svc) {
    if (sa == NULL || svc == NULL || sa->count >= MAX_SERVICES) {
        return -1;
    }

    // 检查是否已存在同名服务
    if (hd_service_array_find_index_by_name(sa, svc->name) != -1) {
        return -1;
    }

    // 复制服务数据
    memcpy(&sa->services[sa->count], svc, sizeof(HDService));
    sa->count++;
    return 0;
}

// 根据名称删除服务 (返回0成功，-1失败)
int hd_service_array_remove_by_name(HDServiceArray *sa, const char *name) {
    int index = hd_service_array_find_index_by_name(sa, name);
    return hd_service_array_remove_by_index(sa, index);
}

// 根据索引删除服务 (返回0成功，-1失败)
int hd_service_array_remove_by_index(HDServiceArray *sa, int index) {
    if (sa == NULL || index < 0 || index >= sa->count) {
        return -1;
    }

    // 将后面的元素前移
    for (int i = index; i < sa->count - 1; i++) {
        memcpy(&sa->services[i], &sa->services[i + 1], sizeof(HDService));
    }

    sa->count--;
    memset(&sa->services[sa->count], 0, sizeof(HDService)); // 清空最后一个元素
    return 0;
}

// 检查服务依赖是否满足
int hd_service_check_dependencies(const HDServiceArray *sa, const HDService *svc) {
    if (sa == NULL || svc == NULL) return -1;

    for (int i = 0; i < svc->depends_on_count; i++) {
        const char *dep_name = svc->depends_on[i];
        int found = 0;
        
        // 检查依赖服务是否存在且正在运行
        for (int j = 0; j < sa->count; j++) {
            if (strcmp(sa->services[j].name, dep_name) == 0 && sa->services[j].is_running) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            return 0; // 依赖不满足
        }
    }
    return 1; // 所有依赖都满足


}


void hd_service_cleanup(HDService *svc) {
    if (svc == NULL) return;
    // 清除所有字段
    memset(svc->name, 0, LEN_SERVICE_NAME);
    memset(svc->path, 0, LEN_SERVICE_PATH);
    memset(svc->version, 0, LEN_SERVICE_VERSION);
    svc->pid = 0;
    svc->is_running = 0;
    svc->last_modified = 0;
    svc->is_main = 0;
    svc->is_secondary = 0;
    svc->is_registered = 0;
    
    // 清理依赖项
    for (int i = 0; i < svc->depends_on_count; i++) {
        memset(svc->depends_on[i], 0, LEN_SERVICE_NAME);
    }
    svc->depends_on_count = 0;
}



// 回收整个服务数组资源
void hd_service_array_cleanup(HDServiceArray *sa) {
    if (sa == NULL) return;
    
    // 清理每个服务
    for (int i = 0; i < sa->count; i++) {
        hd_service_cleanup(&(sa->services[i]));
    }
    
    // 重置数组
    sa->count = 0;
}