#ifndef __HD_INIT__
#define __HD_INIT__

#include <sys/types.h>

#define LEN_SERVICE_NAME    50
#define LEN_SERVICE_PATH    256
#define LEN_SERVICE_VERSION 50
#define MAX_SERVICE_DEPENDS 5
#define MAX_SERVICES 20
#define VERSION "1.0.0"
#define SOCKET_PATH "/tmp/hd_init_socket"

typedef struct {
    char name[LEN_SERVICE_NAME];
    char path[LEN_SERVICE_PATH];
    char version[LEN_SERVICE_VERSION];
    pid_t pid;
    int is_running;
    time_t last_modified;
    int is_main;
    int is_secondary;
    int is_registered;
    int depends_on_count;
    char depends_on[MAX_SERVICE_DEPENDS][LEN_SERVICE_NAME];
} Service;

void list_services(Service*services);

#endif // __HD_INIT__