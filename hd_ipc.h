#ifndef __HD_IPC__
#define __HD_IPC__

#include <stdio.h>
#include "hd_context.h"

#define HD_IPC_VERSION 1

#if HD_CONTEXT

#define HD_IPC_SOCKET_PATH_FOR_CHILD "/Users/xiangpengle/Downloads/hdinit/hd_ipc_socket_for_execl_in_child"
#define HD_IPC_SOCKET_PATH "/Users/xiangpengle/Downloads/hdinit/hd_ipc_socket"

#else

#define HD_IPC_SOCKET_PATH_FOR_CHILD "/root/hdinit/hd_ipc_socket_for_execl_in_child"
#define HD_IPC_SOCKET_PATH "/root/hdinit/hd_ipc_socket"

#endif

#define HD_IPC_SOCKET_PATH_FOR_CHILD_BUFF_SIZE 64
#define HD_IPC_SOCKET_PATH_BUFF_SIZE 4096

const char * ipc_print_help_str() ;

void ipc_print_help() ;

/**
 * @brief 构建一个 JSON 字符串，格式如 {"cmd":"print","param":{"msg":"..."}}
 * @param msg 要打印的消息内容
 * @param json_result 存放生成的 JSON 字符串
 * @return 成功返回 0，失败返回 -1
 */
int hd_ipc_json_print(const char * msg,char * json_result);


/**
 * @brief 构建一个 JSON 字符串，格式如 {"cmd":"error","param":{"error":"...","code":1}}
 * @param msg 要打印的消息内容
 * @param json_result 存放生成的 JSON 字符串
 * @return 成功返回 0，失败返回 -1
 */
int hd_ipc_json_error(const char * error, int code,char * json_result);

int hd_ipc_json_resp_check_result(
    const char *url,
    const char *md5,
    const char *filename,
    const char *service,
    const char *version,
    char *json_result
);


#endif // __HD_IPC__
