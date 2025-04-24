
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

volatile static sig_atomic_t g_running = 1;
static int g_index = 0;

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

static void   ipc_service_on_exit_internal (){
    exit_from_parent();
    g_running = 0;
 }
 
 static void  ipc_service_on_heartbeat_ping_internal (int index){
    g_index = index;
 }


/**

 * gcc -o ./.service/hdmain hd_main.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c  hd_ipc_service.c hd_ipc_client.c hd_ipc_protocol.c  -lcurl
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
    ipc_service_init("hdmain",getpid(),VERSION,ipc_service_on_exit_internal,ipc_service_on_heartbeat_ping_internal);
    int index = 0;
    while (g_running)
    {
        time_t now = time(NULL);
        HD_LOGGER_INFO(TAG, "%s Main-Service-Running ... ... ...%d-%d\n", PREFIX, index++,g_index);
        sleep(10);
    }
    ipc_service_destory();
    HD_LOGGER_INFO(TAG, "%s Main service stopped !!!\n", PREFIX);
    return 0;
}
