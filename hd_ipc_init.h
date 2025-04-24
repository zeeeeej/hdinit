#ifndef __HD_IPC_INIT__
#define __HD_IPC_INIT__

#include "hd_ipc_protocol.h"
#include "cJSON.h"
#include "time.h"

/**
 * 废弃
 * 收到心跳回调
 */
typedef void (*ipc_init_on_heartbeat_pong)(const char *service_name, int index);

/**
 * 废弃
 * 收到shell确定升级回调
 */
typedef void (*ipc_init_on_shell_confirm_upgrade)(const char *service_name);

/**
 * 心跳lost回调
 */
typedef void (*ipc_init_on_heartbeat_lost)(const char *service_name, int index, time_t time);

/**
 * 服务已连接回调
 */
typedef void (*ipc_init_on_connected)(const char *service_name, int service_pid, const char *service_version);

/**
 * @brief 初始化IPC通信模块
 *
 * @param call_ipc_init_on_connected 连接建立时的回调函数
 * @param callback_ipc_on_init_heartbeat_lost 心跳丢失时的回调函数
 * @param callback_heartbeat 心跳响应回调函数
 * @param callback_upgrade_resp 升级确认响应回调函数
 * @return int 返回初始化状态，0表示成功，非0表示错误码
 */
int ipc_init_initialize(
    ipc_init_on_connected call_ipc_init_on_connected,
    ipc_init_on_heartbeat_lost callback_ipc_on_init_heartbeat_lost,
    ipc_init_on_heartbeat_pong callback_heartbeat,
    ipc_init_on_shell_confirm_upgrade callback_upgrade_resp);

/**
 * @brief 反初始化IPC通信模块
 */
void ipc_init_destory();

#endif // __HD_IPC_INIT__
