#ifndef __HD_IPC_PROTOCOL__
#define __HD_IPC_PROTOCOL__

#include <stdio.h>
#include "hd_context.h"

#define HD_IPC_JSON_DEBUG   0
#define HD_IPC_VERSION_CODE 1

#define PORT_INIT 2345  // the port users will be connecting to
#define PORT_SHELL 2344  // the port users will be connecting to
#define PORT_SERVICE 2343  // the port users will be connecting to
 
#define CMD_ipc_core_service_started            "ipc_core_service_started"
#define CMD_ipc_core_heartbeat_ping             "ipc_core_heartbeat_ping"
#define CMD_ipc_core_heartbeat_pong             "ipc_core_heartbeat_pong"
#define CMD_ipc_core_exit_child_execl           "ipc_core_exit_child_execl"
#define CMD_ipc_shell_msg                       "ipc_shell_msg"
#define CMD_ipc_shell_error                     "ipc_shell_error"
#define CMD_ipc_shell_confirm_upgrade           "ipc_shell_confirm_upgrade"
#define CMD_ipc_shell_confirm_upgrade_resp      "ipc_shell_confirm_upgrade_resp"
#define CMD_ipc_shell_download_progress         "ipc_shell_download_progress"

#endif // __HD_IPC_PROTOCOL__
