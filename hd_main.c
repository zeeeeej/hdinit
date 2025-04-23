
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include "hd_logger.h"
#include "hd_ipc.h"
#include "hd_utils.h"
#include "hd_ipc_service.h"

#define TAG "hdmain"
#define PREFIX "%%%%%%"
#define VERSION "0.0.3"

volatile sig_atomic_t g_running = 1;

void handle_signal(int sig)
{
    HD_LOGGER_INFO(TAG, "%s %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% \n", PREFIX);
    HD_LOGGER_INFO(TAG, "%s Main service handle_signal (PID: %d) sig=%d\n", PREFIX, getpid(), sig);
    HD_LOGGER_INFO(TAG, "%s %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% \n", PREFIX);
    if (sig == SIGUSR1)
    {
    }
    else if (sig == SIGUSR2)
    {
        g_running = 0;
    }
    else
    {

    }
}

static void exit_from_parent()
{
    HD_LOGGER_INFO(TAG, "%s Main service exit! %s\n", PREFIX);
    HD_LOGGER_INFO(TAG, "%s Main service exit! %s\n", PREFIX);
    HD_LOGGER_INFO(TAG, "%s Main service exit! %s\n", PREFIX);
}

static cJSON*   p1 (const char * service_name,int index){
    return NULL;
}

static cJSON*   p2 (const char * service_name){
    return NULL;
}


/**

 * gcc -o ./.service/hdmain hd_main.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c  hd_ipc_service.c hd_ipc_client.c   -lcurl
 */
int main(int argc, const char *argv[])
{
    if (HD_DEBUG)
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    }
    else
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_INFO);
    }
    HD_LOGGER_INFO(TAG, "%s Main-Service-Started (PID: %d)\n", PREFIX, getpid());
    ipc_service_init("hdmain",getpid(),VERSION,p1,p2);
    int index = 0;
    while (g_running)
    {
        time_t now = time(NULL);
        HD_LOGGER_INFO(TAG, "%s Main-Service-Running ... ... ...%d\n", PREFIX, index++);
        sleep(10);
    }
    ipc_service_destory();
    HD_LOGGER_INFO(TAG, "%s Main service stopped !!!\n", PREFIX);
    return 0;
}
