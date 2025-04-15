#ifndef __HD_SERVICE_INTERFACE__
#define __HD_SERVICE_INTERFACE__

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct
{
    void (*hd_service_on_init)(void);

    void (*hd_service_on_start)(void);

    void (*hd_service_on_destory)(void);
} hd_service_interface;

extern int hd_service_interface_running;

// 声明函数类型
typedef void (*hd_service_interface_on_parent_exit_child)(void);

/**
 * socket_fd        : unix domain socket parent fd
 * service_name     : service name
 * version          : service version
 * heart_beat       : heart beat interval
 * hd_service_interface_on_parent_exit_child    :   parent exit child callback
 */
int hd_service_interface_init(
    const char *socket_fd,
    const char *service_name,
    const char *version,
    int heart_beat,
    hd_service_interface_on_parent_exit_child callback);

void hd_service_interface_destory();

#endif // __HD_SERVICE_INTERFACE__