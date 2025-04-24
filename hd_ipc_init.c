#include <stdio.h>
#include "hd_ipc_init.h"
#include <stdlib.h>
#include "hd_utils.h"
#include <unistd.h>
#include "hd_logger.h"
#include <pthread.h>
#include <hd_safe_map.h>
#include <stdbool.h>
#include <time.h>
#include <jsonrpc-c.h>

#define TAG "hd_ipc_init"

static void ipc_init_on_heartbeat_lost_internal(const char *name, int index, time_t time, int diff);

/** HeartbeatManager aaaaaaa */
typedef struct
{
    pthread_t timeout_thread;
    pthread_mutex_t mutex;
    bool running;
    time_t last_heartbeat_time;
    char *name;
    int index;
} HeartbeatManager;

// 超时检测线程函数
static void *timeout_monitor(void *arg)
{
    HeartbeatManager *manager = (HeartbeatManager *)arg;
    const int timeout_seconds = 10; // 10秒超时

    while (manager->running)
    {
        pthread_mutex_lock(&manager->mutex);

        time_t current_time = time(NULL);
        int diff = difftime(current_time, manager->last_heartbeat_time);
        if (diff > timeout_seconds)
        {
            // 超时
            ipc_init_on_heartbeat_lost_internal(manager->name, manager->index, manager->last_heartbeat_time, diff);
            manager->running = false; // 超时后停止管理器
        }

        pthread_mutex_unlock(&manager->mutex);
        sleep(1); // 每秒检查一次
    }

    return NULL;
}

// 初始化心跳管理器
HeartbeatManager *heartbeat_init(const char *service_name)
{
    HeartbeatManager *manager = malloc(sizeof(HeartbeatManager));
    if (!manager)
        return NULL;

    manager->running = false;
    manager->index = 0;
    manager->name = strdup(service_name);
    manager->last_heartbeat_time = 0;
    pthread_mutex_init(&manager->mutex, NULL);

    return manager;
}

// 启动心跳管理器
void heartbeat_start(HeartbeatManager *manager)
{
    if (!manager || manager->running)
        return;

    pthread_mutex_lock(&manager->mutex);
    manager->running = true;
    manager->last_heartbeat_time = time(NULL);
    pthread_mutex_unlock(&manager->mutex);

    pthread_create(&manager->timeout_thread, NULL, timeout_monitor, manager);
}

// 停止心跳管理器
void heartbeat_stop(HeartbeatManager *manager)
{
    if (!manager)
        return;

    pthread_mutex_lock(&manager->mutex);
    manager->running = false;
    pthread_mutex_unlock(&manager->mutex);

    pthread_join(manager->timeout_thread, NULL);
}

// 接收到心跳并回复
void heartbeat_received(HeartbeatManager *manager)
{
    if (!manager)
        return;

    pthread_mutex_lock(&manager->mutex);
    // 1. 取消上一次超时任务(通过更新最后心跳时间)
    manager->last_heartbeat_time = time(NULL);

    pthread_mutex_unlock(&manager->mutex);
}

// 释放心跳管理器
void heartbeat_free(HeartbeatManager *manager)
{
    if (!manager)
        return;

    heartbeat_stop(manager);
    pthread_mutex_destroy(&manager->mutex);
    free(manager);
}

/** HeartbeatManager zzzzzzzzz */

static struct jrpc_server my_server;

static ipc_init_on_heartbeat_lost g_ipc_init_heartbeat_lost = NULL;
static ipc_init_on_connected g_ipc_init_on_connected = NULL;
static ipc_init_on_heartbeat_pong g_callback_heartbeat = NULL;
static ipc_init_on_shell_confirm_upgrade g_callback_upgrade_resp = NULL;

#define HEART_BEAT_INTERNAL 5

static ThreadSafeMap *g_map = NULL;

static cJSON *ipc_resp_cmd_ipc_core_heartbeat_pong(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    if (g_callback_heartbeat)
    {
        // 解析数据
        char name[1024] = {0};
        int index = 0;
        cJSON *name_item = cJSON_GetObjectItemCaseSensitive(params, "name");
        if ((cJSON_IsString(name_item)) && (name_item->valuestring != NULL))
        {
            snprintf(name, sizeof(name), "%s", name_item->valuestring);
        }
        else
        {
            return NULL;
        }

        cJSON *index_item = cJSON_GetObjectItemCaseSensitive(params, "index");
        if (cJSON_IsNumber(index_item))
        {
            index = index_item->valueint;
        }
        else
        {
            return NULL;
        }

        // * 收到心跳数据 * //
        void *value = hd_map_get(g_map, name);
        if (value == NULL)
        {
            return NULL;
        }
        HeartbeatManager *data = (HeartbeatManager *)value;
        if (data != NULL)
        {
            int last_index = data->index;
            if (last_index + 1 == index)
            {
                // 更新
                data->index++;
                heartbeat_received(data);
                HD_LOGGER_INFO(TAG, "++++++ pong ++++++ %s %d %ld ++++++ pong ++++++ \n\n", data->name, index, data->last_heartbeat_time);
                sleep(HEART_BEAT_INTERNAL);
                cJSON *result = ipc_request_core_heartbeat_ping(data->name, last_index + 1);
                if (result != NULL && g_callback_heartbeat)
                {
                    g_callback_heartbeat(data->name, last_index + 1);
                }

                return result;
            }
            else
            {
                HD_LOGGER_ERROR(TAG, "hd_ipc_init.c|ipc_resp_cmd_ipc_core_heartbeat_pong|服务:%s index不匹配 %d(new) != %d(old)\n", name, index, last_index);
            }
        }
    }
    return NULL;
}

static int start_heartbeat_task(const char *name, const char *version, int pid)
{
    if (name == NULL || version == NULL)
    {
        HD_LOGGER_ERROR(TAG, "<start_heartbeat_task> check fail \n");
        return -1;
    }

    // 1.删除老的
    int ret;
    HeartbeatManager *data = NULL;
    HD_LOGGER_DEBUG(TAG, "<start_heartbeat_task> remove old ...\n");
    while (1)
    {
        hd_map_print_all(g_map, "=");
        void *value = hd_map_get(g_map, name);
        if (value != NULL)
        {
            data = (HeartbeatManager *)value;
        }
        else
        {
            data = NULL;
        }

        if (data != NULL)
        {
            HD_LOGGER_DEBUG(TAG, ">>>> 等待任务:%s删除 ... ...<<<< \n", name);
            heartbeat_stop(data);
            heartbeat_free(data);
            hd_map_remove(g_map, name);
            HD_LOGGER_DEBUG(TAG, ">>>> 等待任务:%s删除完毕 ！！！<<<< \n", name);
            sleep(1);
        }
        else
        {
            HD_LOGGER_DEBUG(TAG, ">>>> 不存在 ！！！<<<< \n");
            break;
        }
    }

    // 2.添加新的
    HD_LOGGER_DEBUG(TAG, "<start_heartbeat_task> add new ...\n");
    HeartbeatManager *new_data = heartbeat_init(name);
    heartbeat_start(new_data);
    ret = hd_map_put(g_map, name, new_data);
    if (!ret)
    {
        HD_LOGGER_ERROR(TAG, "<start_heartbeat_task> hd_map_put g_map=%p name=%s data=%p failed \n", g_map, name, new_data);
        return -1;
    }
    HD_LOGGER_DEBUG(TAG, "<start_heartbeat_task> add new ok.\n");
    return new_data->index;
}

static cJSON *ipc_resp_cmd_ipc_core_service_started(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    if (HD_IPC_JSON_DEBUG)
    {
        hd_ipc_print_cjson(params, "<ipc_resp_cmd_ipc_core_service_started>");
    }

    char name[1024] = {0};
    char version[1024] = {0};
    int pid = 0;

    cJSON *name_item = cJSON_GetObjectItemCaseSensitive(params, "name");
    if ((cJSON_IsString(name_item)) && (name_item->valuestring != NULL))
    {
        snprintf(name, sizeof(name), "%s", name_item->valuestring);
    }
    else
    {
        return NULL;
    }
    cJSON *version_item = cJSON_GetObjectItemCaseSensitive(params, "version");
    if ((cJSON_IsString(version_item)) && (version_item->valuestring != NULL))
    {
        snprintf(version, sizeof(version), "%s", version_item->valuestring);
    }
    else
    {
        return NULL;
    }
    cJSON *pid_item = cJSON_GetObjectItemCaseSensitive(params, "pid");
    if ((cJSON_IsNumber(pid_item)))
    {
        pid = pid_item->valueint;
    }
    else
    {
        return NULL;
    }
    // 启动心跳任务
    HD_LOGGER_DEBUG(TAG, "start_heartbeat_task %s %s %d ...... \n", name, version, pid);
    int index = start_heartbeat_task(name, version, pid);
    HD_LOGGER_DEBUG(TAG, "start_heartbeat_task %s %s %d result:%d \n", name, version, pid, index);
    if (index < 0)
    {
        return NULL;
    }

    if (g_ipc_init_on_connected)
    {
        g_ipc_init_on_connected(name, pid, version);
    }

    HD_LOGGER_INFO(TAG, "+++ 服务[%s](%s)-%d已启动！ +++\n\n", name, version, pid);
    return ipc_request_core_heartbeat_ping(name, index);
    return NULL;
}

static cJSON *ipc_resp_cmd_ipc_shell_confirm_upgrade_resp(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    return NULL;
}

static void ipc_init_on_heartbeat_lost_internal(const char *name, int index, time_t time, int diff)
{
    HD_LOGGER_ERROR(TAG, "%s timeout !!!!!! index=%d time=%ld diff=%d\n", name, index, time, diff);
    if (g_ipc_init_heartbeat_lost)
    {
        g_ipc_init_heartbeat_lost(name, index, time);
    }
}

int ipc_init_initialize(
    ipc_init_on_connected call_ipc_init_on_connected,
    ipc_init_on_heartbeat_lost callback_ipc_on_init_heartbeat_lost,
    ipc_init_on_heartbeat_pong callback_heartbeat,
    ipc_init_on_shell_confirm_upgrade callback_upgrade_resp)
{
    HD_LOGGER_INFO(TAG, "<ipc_init_initialize>");
    g_map = hd_map_init(20);
    g_ipc_init_on_connected = call_ipc_init_on_connected;
    g_ipc_init_heartbeat_lost = callback_ipc_on_init_heartbeat_lost;
    g_callback_heartbeat = callback_heartbeat;
    g_callback_upgrade_resp = callback_upgrade_resp;
    jrpc_server_init(&my_server, PORT_INIT);
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_core_heartbeat_pong, CMD_ipc_core_heartbeat_pong, NULL);
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_core_service_started, CMD_ipc_core_service_started, NULL);
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_shell_confirm_upgrade_resp, CMD_ipc_shell_confirm_upgrade_resp, NULL);
    jrpc_server_run(&my_server);
    return 0;
}

static void cancelHeartbeatThread(void *value, void *context)
{
    HeartbeatManager *data = (HeartbeatManager *)value;
    if (data == NULL)
    {
        return;
    }
    heartbeat_stop(data);
    heartbeat_free(data);
}

static void cancelHeartbeatThreads()
{
    hd_map_for_each(g_map, cancelHeartbeatThread, NULL);
}

void ipc_init_destory()
{
    HD_LOGGER_INFO(TAG, "<ipc_init_destory>");
    g_ipc_init_on_connected = NULL;
    g_callback_heartbeat = NULL;
    g_callback_upgrade_resp = NULL;
    g_ipc_init_heartbeat_lost = NULL;
    hd_map_deinit(g_map);
    g_map = NULL;
    jrpc_server_destroy(&my_server);
}
