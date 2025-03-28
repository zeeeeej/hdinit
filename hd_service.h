#ifndef __HD_SERVICE__
#define __HD_SERVICE__

#include <sys/types.h>

#define LEN_SERVICE_NAME    50
#define LEN_SERVICE_PATH    256
#define LEN_SERVICE_VERSION 50
#define MAX_SERVICE_DEPENDS 5
#define MAX_SERVICES 20

typedef struct {
    char name[LEN_SERVICE_NAME];
    char path[LEN_SERVICE_PATH];
    char version[LEN_SERVICE_VERSION];
    pid_t pid;
    int is_running;
    time_t last_modified;
    int is_main;
    int is_secondary;
    int is_registered;
    int depends_on_count;
    char depends_on[MAX_SERVICE_DEPENDS][LEN_SERVICE_NAME];
} HDService;

typedef struct {
    HDService services[MAX_SERVICES];
    int count;
} HDServiceArray;

// 初始化服务数组
void hd_service_array_init(HDServiceArray *sa);

// 打印单个服务信息
void hd_service_print(const HDService *svc);

// 打印所有服务信息
void hd_service_array_print(const HDServiceArray *sa);

// 根据名称查找服务索引 (-1表示未找到)
int hd_service_array_find_index_by_name(const HDServiceArray *sa, const char *name);

// 根据名称查找服务指针 (NULL表示未找到)
HDService* hd_service_array_find_by_name(HDServiceArray *sa, const char *name);

// 根据索引获取服务指针 (NULL表示无效索引)
HDService* hd_service_array_get_by_index(HDServiceArray *sa, int index);

// 添加服务 (返回0成功，-1失败)
int hd_service_array_add(HDServiceArray *sa, const HDService *svc);

// 根据名称删除服务 (返回0成功，-1失败)
int hd_service_array_remove_by_name(HDServiceArray *sa, const char *name);

// 根据索引删除服务 (返回0成功，-1失败)
int hd_service_array_remove_by_index(HDServiceArray *sa, int index);

// 检查服务依赖是否满足
int hd_service_check_dependencies(const HDServiceArray *sa, const HDService *svc);

void hd_service_array_cleanup(HDServiceArray *sa) ;

#endif // __HD_SERVICE__