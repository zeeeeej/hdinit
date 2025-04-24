#include "hd_ipc_service.h"
#include "hd_ipc_client.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "hd_utils.h"
#include "hd_logger.h"
#include "hd_ipc_protocol.h"

#define TAG "hd_ipc_service"

static ipc_service_on_heartbeat_ping g_ipc_service_on_heartbeat_ping = NULL;
static ipc_service_on_exit g_ipc_service_on_exit = NULL;

static char g_service_name[1024] = {0};
static int g_pid = 0;
static char g_version[1024] = {0};

static void ipc_client_recv_func_internal(cJSON *data)
{

    hd_ipc_print_cjson(data, "ipc_client_recv_func_internal 解析method");
    // 解析开始
    // {
    //     "method":	"ipc_core_heartbeat_ping",
    //     "jsonrpc":	"jsonrpc",
    //     "id":	0,
    //     "params":	{
    //         "name":	"hdmain",
    //         "index":	0
    //     }
    // }
    char method[1024] = {0};
    cJSON *method_item = cJSON_GetObjectItemCaseSensitive(data, IPC_JSON_KEY_METHOD);
    if (cJSON_IsString(method_item) && (method_item->valuestring != NULL))
    {
        snprintf(method, sizeof(method), "%s", method_item->valuestring);
    }
    else
    {
        HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> method_item fail.\n");
        return;
    }

    cJSON *params_item = cJSON_GetObjectItemCaseSensitive(data, IPC_JSON_KEY_PARAMS);
    if (cJSON_IsNull(params_item))
    {
        HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> params_item fail.\n");
        return;
    }

    // method = ipc_core_heartbeat_ping
    if (strcmp(CMD_ipc_core_heartbeat_ping, method) == 0)
    {
        char name[1024] = {0};
        int index = 0;
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(params_item, "name");
        if (cJSON_IsString(name_item) && (name_item->valuestring != NULL))
        {
            snprintf(name, sizeof(name), "%s", name_item->valuestring);
        }
        else
        {
            HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> name_item fail.\n");
            return;
        }

        cJSON *index_item = cJSON_GetObjectItemCaseSensitive(params_item, "index");
        if (cJSON_IsNumber(index_item))
        {
            index = index_item->valueint;
        }
        else
        {
            HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> index_item fail.\n");
            return;
        }

        if (strcmp(g_service_name, name) != 0)
        {
            HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> service not same.\n");
            return;
        }

        HD_LOGGER_ERROR(TAG, "------ ping ------ %s %d ------ ping ------\n\n", name, index);
        // 关闭child
        if (strcmp(g_service_name, name) == 0)
        {
            if (g_ipc_service_on_heartbeat_ping)
            {
                g_ipc_service_on_heartbeat_ping(index);
            }
        }
        // 根据cmd处理数据
        cJSON *resp_data = ipc_request_core_heartbeat_pong(name, index + 1);
        if (resp_data)
        {
            ipc_client_send(resp_data);
        }
    }

    // method = ipc_core_exit_child_execl
    else if (strcmp(CMD_ipc_core_exit_child_execl, method) == 0)
    {
        char name[1024] = {0};
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(params_item, "name");
        if (cJSON_IsString(name_item) && (name_item->valuestring != NULL))
        {
            snprintf(name, sizeof(name), "%s", name_item->valuestring);
        }
        else
        {
            HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> name_item fail.\n");
            return;
        }

        // 关闭child
        if (strcmp(g_service_name, name) == 0)
        {
            if (g_ipc_service_on_exit)
            {
                g_ipc_service_on_exit();
            }
        }
    }
}

static void *recv_thread(void *arg)
{
    ipc_client_recv(ipc_client_recv_func_internal);
    return NULL;
}

int ipc_service_init(
    const char *service_name,
    int pid,
    const char *version,
    ipc_service_on_exit ipc_service_on_exit_callback,
    ipc_service_on_heartbeat_ping ipc_service_on_heartbeat_ping_callback)
{
    if (service_name == NULL || version == NULL)
    {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init> service_name or version error .\n");
        return -1;
    }

    int ret;
    memset(g_service_name, 0, sizeof(g_service_name));
    g_pid = 0;
    memset(g_version, 0, sizeof(g_version));

    strncpy(g_service_name, service_name, sizeof(g_service_name) - 1);
    g_service_name[sizeof(g_service_name) - 1] = '\0';

    g_pid = pid;

    strncpy(g_version, version, sizeof(g_version) - 1);
    g_version[sizeof(g_version) - 1] = '\0';

    g_ipc_service_on_exit = ipc_service_on_exit_callback;
    g_ipc_service_on_heartbeat_ping = ipc_service_on_heartbeat_ping_callback;
    int index = 0;
    while (1)
    {
        ret = ipc_client_initialize("127.0.0.1", PORT_INIT);
        if (ret == 0 || index > 5)
        {
            break;
        }
        index++;
        sleep(1);
    }

    if (ret != 0)
    {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_client_initialize error %d .\n", ret);
        return -1;
    }

    pthread_t t;
    pthread_create(&t, NULL, recv_thread, NULL);

    HD_LOGGER_INFO(TAG, "<ipc_service_init> %s %d %s\n", service_name, pid, version);

    cJSON *resp_data = ipc_request_core_service_started(service_name, pid, version);
    if (resp_data)
    {
        ret = ipc_client_send(resp_data);
        if (ret != 0)
        {
            HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_client_send error %d .\n", ret);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_request_core_service_started error %d .\n", ret);
    }
    // pthread_join(t,NULL);

    return 0;
}

void ipc_service_destory()
{
    HD_LOGGER_INFO(TAG, "<ipc_service_destory> %s %d %s\n", g_service_name, g_pid, g_version);
    ipc_client_destory();
    memset(g_service_name, 0, sizeof(g_service_name));
    g_pid = 0;
    memset(g_version, 0, sizeof(g_version));
}
