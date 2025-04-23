#ifndef __HD_IPC_SHELL__
#define __HD_IPC_SHELL__

#include "hd_ipc_protocol.h"
#include "cJSON.h"

int ipc_shell_init();
void ipc_shell_destory();
int ipc_shell_confirm_upgrade_resp(int result);

#endif // __HD_IPC_SHELL__
