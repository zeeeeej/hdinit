
#include "hd_ipc_shell.h"
#include <jsonrpc-c.h>
#include "hd_ipc_client.h"


int ipc_shell_init(){
    ipc_client_initialize();
    return 0;
}
void ipc_shell_destory(){
    ipc_client_destory();
}

int ipc_shell_confirm_upgrade_resp(int result){
    return 0;
}

