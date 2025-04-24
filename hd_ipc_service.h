#ifndef __HD_IPC_SERVICE__
#define __HD_IPC_SERVICE__

#include "hd_ipc_protocol.h"
#include "cJSON.h"

typedef void (*ipc_service_on_exit)(void );
typedef void (*ipc_service_on_heartbeat_ping)(int index );

int ipc_service_init(
    const char *service_name,
    int pid,
    const char *version,
    ipc_service_on_exit ipc_service_on_exit_callback,
    ipc_service_on_heartbeat_ping ipc_service_on_heartbeat_ping_callback
);

void ipc_service_destory();

#endif // __HD_IPC_SERVICE__
