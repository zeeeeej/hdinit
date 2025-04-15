#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"


const char *ipc_print_help_str() {
    static const char *help_msg = 
        "hdinit commands:\n"
        "  -v                        Show version\n"
        "  -l                        List all services\n"
        "  -d <service>              Show service details\n"
        "  -h                        Show this help\n"
        "  start <service>           Start a service\n"
        "  stop  <service>           Stop a service\n"
        "  check <service>           Check if service has updates\n"
        "  register   <name> <path>  Register a new service\n"
        "  unregister <name>         Unregister a service\n"
        "  -s                        Shutdown hdinit\n";
    return help_msg;
}

void ipc_print_help() {
    printf("%s",ipc_print_help_str())      ;
}

   
    
    
int hd_ipc_json_print(const char *msg, char *json_result) {
    if (msg == NULL || json_result == NULL) {
        return -1;  // 参数检查
    }

    cJSON *root = cJSON_CreateObject();  // 创建根 JSON 对象
    if (root == NULL) {
        return -1;  // 内存分配失败
    }

    // 添加 "cmd": "print" 字段
    cJSON_AddStringToObject(root, "cmd", "print");

    // 创建嵌套的 "param" 对象，并添加 "msg": msg 字段
    cJSON *param = cJSON_CreateObject();
    if (param == NULL) {
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddStringToObject(param, "msg", msg);
    cJSON_AddItemToObject(root, "param", param);

    // 生成 JSON 字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    // 确保 json_result 有足够空间，并复制结果
    strncpy(json_result, json_str, strlen(json_str) + 1);

    // 释放内存
    free(json_str);
    cJSON_Delete(root);

    return 0;  // 成功
}


int hd_ipc_json_error(const char *error,int code, char *json_result) {
    if (error == NULL || json_result == NULL) {
        return -1;  // 参数检查
    }

    cJSON *root = cJSON_CreateObject();  // 创建根 JSON 对象
    if (root == NULL) {
        return -1;  // 内存分配失败
    }

    // 添加 "cmd": "error" 字段
    cJSON_AddStringToObject(root, "cmd", "error");

    // 创建嵌套的 "param" 对象，并添加 "error" "code" 字段
    cJSON *param = cJSON_CreateObject();
    if (param == NULL) {
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddStringToObject(param, "error", error);
    cJSON_AddNumberToObject(param, "code", code);
    cJSON_AddItemToObject(root, "param", param);

    // 生成 JSON 字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    // 确保 json_result 有足够空间，并复制结果
    strncpy(json_result, json_str, strlen(json_str) + 1);

    // 释放内存
    free(json_str);
    cJSON_Delete(root);

    return 0;  // 成功
}

/**
 * {
        "cmd": "check_result",
        "param": {
            "url": "xxxxxxx",
            "md5":"xxx",
            "filename":"",
            "service":"hdmain",
            "version":"1.0.0"
        }
    }
 */
int hd_ipc_json_resp_check_result(
    const char *url,
    const char *md5,
    const char *filename,
    const char *service,
    const char *version,
    char *json_result
) {
    if (url == NULL ||md5 == NULL || filename== NULL ||service== NULL ||version== NULL ||json_result == NULL) {
        return -1;  // 参数检查
    }

    cJSON *root = cJSON_CreateObject();  // 创建根 JSON 对象
    if (root == NULL) {
        return -1;  // 内存分配失败
    }

    cJSON_AddStringToObject(root, "cmd", "check_result");

    cJSON *param = cJSON_CreateObject();
    if (param == NULL) {
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddStringToObject(param, "url", url);
    cJSON_AddStringToObject(param, "md5", md5);
    cJSON_AddStringToObject(param, "version", version);
    cJSON_AddStringToObject(param, "service", service);
    cJSON_AddStringToObject(param, "filename", filename);

    cJSON_AddItemToObject(root, "param", param);

    // 生成 JSON 字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    // 确保 json_result 有足够空间，并复制结果
    strncpy(json_result, json_str, strlen(json_str) + 1);

    // 释放内存
    free(json_str);
    cJSON_Delete(root);

    return 0;  // 成功
}


int hd_ipc_json_resp_progress(
    int progress,
    char *json_result
) {
    if (json_result == NULL) {
        return -1;  // 参数检查
    }

    cJSON *root = cJSON_CreateObject();  // 创建根 JSON 对象
    if (root == NULL) {
        return -1;  // 内存分配失败
    }

    cJSON_AddStringToObject(root, "cmd", "progress");

    cJSON *param = cJSON_CreateObject();
    if (param == NULL) {
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddNumberToObject(param, "progress", progress);
    cJSON_AddItemToObject(root, "param", param);

    // 生成 JSON 字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    // 确保 json_result 有足够空间，并复制结果
    strncpy(json_result, json_str, strlen(json_str) + 1);

    // 释放内存
    free(json_str);
    cJSON_Delete(root);

    return 0;  // 成功
}