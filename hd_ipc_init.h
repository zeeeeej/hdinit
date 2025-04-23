#ifndef __HD_IPC_INIT__
#define __HD_IPC_INIT__

#include "hd_ipc_protocol.h"
#include "cJSON.h"
#include "time.h"

/**
 * 废弃
 * 收到心跳回调
 */
typedef cJSON*  (*ipc_callback_core_heartbeat_pong)(const char * service_name,int index);
/**
 * 废弃
 * 收到服务启动信息回调
 */
typedef cJSON*  (*ipc_callback_core_service_started)(const char * service_name,int pid,const char * version);
/**
 * 废弃
 * 收到shell确定升级回调
 */
typedef cJSON*  (*ipc_callback_shell_confirm_upgrade_resp)(const char * service_name);

/**
 * 心跳lost回调
 */
typedef void (*ipc_init_on_heartbeat_lost)(const char * service_name,int index,time_t time);

/**
 * 服务已连接回调
 */
typedef void (*ipc_init_on_connected)(const char * service_name,int service_pid,const char * service_version);

/**
 * @brief 初始化IPC通信模块
 * 
 * @param call_ipc_init_on_connected 连接建立时的回调函数
 * @param callback_ipc_on_init_heartbeat_lost 心跳丢失时的回调函数
 * @param callback_heartbeat [已废弃] 心跳响应回调函数
 * @param callback_service_started [已废弃] 服务启动回调函数
 * @param callback_upgrade_resp [已废弃] 升级确认响应回调函数
 * @return int 返回初始化状态，0表示成功，非0表示错误码
 */
int ipc_init_initialize(  
    ipc_init_on_connected call_ipc_init_on_connected,
    ipc_init_on_heartbeat_lost callback_ipc_on_init_heartbeat_lost,
    ipc_callback_core_heartbeat_pong callback_heartbeat,
    ipc_callback_core_service_started callback_service_started,
    ipc_callback_shell_confirm_upgrade_resp callback_upgrade_resp
);

/**
 * @brief 反初始化IPC通信模块
 */
void ipc_init_destory();

/**
 *  @brief [已废弃]发送数据
 */
int ipc_init_send(const cJSON * data);

/**
 * @brief 构建发送出去的ping数据
 * @param service_name 服务名称
 * @param index 序号
 * @return cJSON* 返回json数据
 */
cJSON* ipc_request_core_heartbeat_ping(const char * service_name,int index);

/**
 * @brief 构建发送出去的停止子服务数据
 * @param service_name 服务名称
 * @return cJSON* 返回json数据
 */
cJSON*  ipc_request_core_exit_child_execl(const char * service_name);

/**
 * @brief 构建发送出去的打印消息数据
 * @param msg 打印的消息
 * @return cJSON* 返回json数据
 */
cJSON*  ipc_request_shell_msg(const char * msg);

/**
 * @brief 构建发送出去的错误信息数据
 * @param msg 错误信息描述
 * @param code 错误信息code
 * @return cJSON* 返回json数据
 */
cJSON*  ipc_request_shell_error(const char * error,int code);

/**
 * @brief 构建发送出去的确认升级数据
 * @return cJSON* 返回json数据
 */
cJSON*  ipc_request_shell_confirm_upgrade();

/**
 * @brief 构建发送出去的下载进度数据
 * @param service_name 服务名称
 * @param max 总进度
 * @param max 当前进度
 * @return cJSON* 返回json数据
 */
cJSON*  ipc_request_shell_download_progress(const char * service_name,int max ,int progress);

#endif // __HD_IPC_INIT__
