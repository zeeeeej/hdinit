#ifndef __HD_IPC_SERVICE__
#define __HD_IPC_SERVICE__

#include "hd_ipc_protocol.h"
#include "cJSON.h"

typedef cJSON* (*ipc_callback_core_heartbeat_ping)(const char * service_name,int index);
typedef cJSON*  (*ipc_callback_core_exit_child_execl)(const char * service_name);

int ipc_service_init(
    const char * service_name,
    int pid,
    const char * version,
    ipc_callback_core_heartbeat_ping callback_heartbeat_ping ,
    ipc_callback_core_exit_child_execl callback_exit_child_execl
);
void ipc_service_destory();
cJSON* ipc_core_service_started(const char * service_name,int pid,const char * version);
cJSON* ipc_core_heartbeat_pong(const char * service_name,int index);

#endif // __HD_IPC_SERVICE__
