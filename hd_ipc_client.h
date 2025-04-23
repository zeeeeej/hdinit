#ifndef __HD_IPC_CLIENT__
#define __HD_IPC_CLIENT__

#include "cJSON.h"

typedef void  (*ipc_client_recv_func)(cJSON* data);

int ipc_client_initialize(const char * server_addr,int server_port);

void ipc_client_destory();

int ipc_client_send(cJSON* data);

void ipc_client_recv(ipc_client_recv_func func);

cJSON* ipc_client_create_json(const char * rpc_method,int rpc_id,const char * rpc_version,cJSON * rpc_params);

#endif // __HD_IPC_CLIENT__
