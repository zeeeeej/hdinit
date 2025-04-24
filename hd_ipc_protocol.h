#ifndef __HD_IPC_PROTOCOL__
#define __HD_IPC_PROTOCOL__

#include <stdio.h>
#include "hd_context.h"
#include "cJSON.h"

#define HD_IPC_JSON_DEBUG 0
#define HD_IPC_VERSION_CODE 1

#define PORT_INIT 2345    // the port users will be connecting to
#define PORT_SHELL 2344   // the port users will be connecting to
#define PORT_SERVICE 2343 // the port users will be connecting to

#define CMD_ipc_core_service_started "ipc_core_service_started"
#define CMD_ipc_core_heartbeat_ping "ipc_core_heartbeat_ping"
#define CMD_ipc_core_heartbeat_pong "ipc_core_heartbeat_pong"
#define CMD_ipc_core_exit_child_execl "ipc_core_exit_child_execl"
#define CMD_ipc_shell_msg "ipc_shell_msg"
#define CMD_ipc_shell_error "ipc_shell_error"
#define CMD_ipc_shell_confirm_upgrade "ipc_shell_confirm_upgrade"
#define CMD_ipc_shell_confirm_upgrade_resp "ipc_shell_confirm_upgrade_resp"
#define CMD_ipc_shell_download_progress "ipc_shell_download_progress"

// json keys
#define IPC_JSON_KEY_ID "id"
#define IPC_JSON_KEY_VERSION "jsonrpc"
#define IPC_JSON_KEY_METHOD "method"
#define IPC_JSON_KEY_PARAMS "params"
// json default values
#define IPC_JSON_KEY_ID_DEFAULT 0
#define IPC_JSON_KEY_VERSION_DEFAULT "jsonrpc"
/**
 *  {
 *      "method":"ipc_core_heartbeat_ping",
 *      "jsonrpc":"xxxx",
 *      "id":123,
 *      "params":xxxxx
 *  }
 */
cJSON *ipc_client_create_json(const char *rpc_method, int rpc_id, const char *rpc_version, cJSON *rpc_params);

cJSON *ipc_client_create_json_default(const char *rpc_method, cJSON *rpc_params);

/**
 * @brief 构建心跳Ping请求的JSON数据
 *
 * @details 该函数构建一个符合IPC通信协议的心跳Ping请求JSON对象。
 * 生成的JSON格式为Parent->Child方向的通信格式，包含方法名和参数。
 *
 * @param[in] service_name 服务名称，用于标识心跳来源服务
 * @param[in] index 心跳序号，用于跟踪心跳序列
 *
 * @return cJSON* 成功返回创建的JSON对象指针，失败返回NULL
 * @retval NULL 表示内存分配失败或参数无效
 *
 * @note
 * - 返回的cJSON对象需要调用cJSON_Delete()释放
 * - 服务名称不应为NULL或空字符串
 * - 序号应当单调递增以确保正确性
 *
 * @par 示例:
 * @code
 * cJSON *ping = ipc_request_core_heartbeat_ping("video_service", 42);
 * if (ping) {
 *     char *json_str = cJSON_Print(ping);
 *     printf("%s\n", json_str);
 *     free(json_str);
 *     cJSON_Delete(ping);
 * }
 * @endcode
 *
 * @par JSON格式示例:
 * @code
 * {
 *     "method": "ipc_core_heartbeat_ping",
 *     "params": {
 *         "name": "video_service",
 *         "index": 42
 *     }
 * }
 * @endcode
 *
 * @see cJSON
 * @see cJSON_CreateObject
 * @see cJSON_AddStringToObject
 * @see cJSON_AddNumberToObject
 */
cJSON *ipc_request_core_heartbeat_ping(const char *service_name, int index);

/**
 * @brief 构建发送出去的停止子服务数据
 * @param service_name 服务名称
 * @return cJSON* 返回json数据
 */
cJSON *ipc_request_core_exit_child_execl(const char *service_name);

/**
 * @brief 构建发送出去的打印消息数据
 * @param msg 打印的消息
 * @return cJSON* 返回json数据
 */
cJSON *ipc_request_shell_msg(const char *msg);

/**
 * @brief 构建发送出去的错误信息数据
 * @param msg 错误信息描述
 * @param code 错误信息code
 * @return cJSON* 返回json数据
 */
cJSON *ipc_request_shell_error(const char *error, int code);

/**
 * @brief 构建发送出去的确认升级数据
 * @return cJSON* 返回json数据
 */
cJSON *ipc_request_shell_confirm_upgrade();

/**
 * @brief 构建发送出去的下载进度数据
 * @param service_name 服务名称
 * @param max 总进度
 * @param max 当前进度
 * @return cJSON* 返回json数据
 */
cJSON *ipc_request_shell_download_progress(const char *service_name, int max, int progress);

cJSON *ipc_request_core_service_started(const char *service_name, int pid, const char *version);

cJSON *ipc_request_core_heartbeat_pong(const char *service_name, int index);

#endif // __HD_IPC_PROTOCOL__
