#include <stdio.h>
#include "hd_context.h"
#include "cJSON.h"
#include "hd_ipc_protocol.h"

// base
cJSON *ipc_client_create_json(const char *rpc_method, int rpc_id, const char *rpc_version, cJSON *rpc_params)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        fprintf(stderr, "ipc_request_core_heartbeat_pong() fail.\n");
        return NULL;
    }
    cJSON_AddStringToObject(root, IPC_JSON_KEY_METHOD, rpc_method);
    cJSON_AddStringToObject(root, IPC_JSON_KEY_VERSION, rpc_version);
    cJSON_AddNumberToObject(root, IPC_JSON_KEY_ID, rpc_id);
    cJSON_AddItemToObject(root, IPC_JSON_KEY_PARAMS, rpc_params);
    return root;
}

cJSON *ipc_client_create_json_default(const char *rpc_method, cJSON *rpc_params)
{
    return ipc_client_create_json(rpc_method, IPC_JSON_KEY_ID_DEFAULT, IPC_JSON_KEY_VERSION_DEFAULT, rpc_params);
}

// ...
cJSON *ipc_request_core_heartbeat_ping(const char *service_name, int index)
{
    cJSON *params = cJSON_CreateObject();
    if (!params)
    {
        fprintf(stderr, "<ipc_request_core_heartbeat_ping> fail!\n");
        return NULL;
    }

    cJSON_AddStringToObject(params, "name", service_name);
    cJSON_AddNumberToObject(params, "index", index);
    cJSON *root = ipc_client_create_json_default(CMD_ipc_core_heartbeat_ping, params);
    return root;
}

cJSON *ipc_request_core_exit_child_execl(const char *service_name)
{
    return NULL;
}

cJSON *ipc_request_shell_msg(const char *msg)
{
    return NULL;
}

cJSON *ipc_request_shell_error(const char *error, int code)
{
    return NULL;
}

cJSON *ipc_request_shell_confirm_upgrade()
{
    return NULL;
}

cJSON *ipc_request_shell_download_progress(const char *service_name, int max, int progress)
{
    return NULL;
}

cJSON *ipc_request_core_service_started(const char *service_name, int pid, const char *version)
{
    cJSON *params = cJSON_CreateObject();
    if (!params)
    {
        fprintf(stderr, "<ipc_request_core_service_started> cJSON_CreateObject params fail.\n");
        return NULL;
    }
    cJSON_AddStringToObject(params, "name", service_name);
    cJSON_AddNumberToObject(params, "pid", pid);
    cJSON_AddStringToObject(params, "version", version);
    cJSON *root = ipc_client_create_json_default(CMD_ipc_core_service_started, params);
    return root;
}

cJSON *ipc_request_core_heartbeat_pong(const char *service_name, int index)
{
    cJSON *params = cJSON_CreateObject();
    if (!params)
    {
        fprintf(stderr, "<ipc_request_core_heartbeat_pong> cJSON_CreateObject params fail.\n");
        return NULL;
    }
    cJSON_AddStringToObject(params, "name", service_name);
    cJSON_AddNumberToObject(params, "index", index);
    cJSON *root = ipc_client_create_json_default(CMD_ipc_core_heartbeat_pong, params);
    return root;
}
