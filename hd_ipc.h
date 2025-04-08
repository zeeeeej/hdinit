#ifndef __HD_IPC__
#define __HD_IPC__

#include <stdio.h>

#define HD_IPC_VERSION 1
// #define HD_IPC_SOCKET_PATH "/tmp/hd_ipc_socket"
#define DEBUG 0

#if DEBUG

#define HD_IPC_SOCKET_PATH_FOR_CHILD "/Users/xiangpengle/Downloads/hdinit/hd_ipc_socket_for_execl_in_child"
#define HD_IPC_SOCKET_PATH "/Users/xiangpengle/Downloads/hdinit/hd_ipc_socket"

#else

#define HD_IPC_SOCKET_PATH_FOR_CHILD "/root/hdinit/hd_ipc_socket_for_execl_in_child"
#define HD_IPC_SOCKET_PATH "/root/hdinit/hd_ipc_socket"

#endif

const char *ipc_print_help_str() {
    static const char *help_msg = 
        "hdinit commands:\n"
        "  -v                        Show version\n"
        "  -l                        List all services\n"
        "  -d <service>              Show service details\n"
        "  -h                        Show this help\n"
        "  start <service>           Start a service\n"
        "  stop  <service>           Stop a service\n"
        "  check <service>           Check if service has updates\n"
        "  register   <name> <path>  Register a new service\n"
        "  unregister <name>         Unregister a service\n"
        "  -s                        Shutdown hdinit\n";
    return help_msg;
}

void ipc_print_help() {
    printf("%s",ipc_print_help_str())      ;
}


#endif // __HD_IPC__
