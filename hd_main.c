
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
#include "hd_service_interface.h"

#define TAG "hdmain"
#define PREFIX "%%%%%%"
#define VERSION "0.0.4"

volatile sig_atomic_t running = 1;

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
        running = 0;
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

/**
 * gcc  hd_main.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c hd_service_interface.c -o ./.service/hdmain
 * gcc  hd_main.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -o hd_service_interface.c  ./server/files/hdmain-0.0.5
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
    HD_LOGGER_INFO(TAG, "%s Main service started (PID: %d)\n", PREFIX, getpid());

    hd_service_interface_init(argv[1], "hdmain", VERSION, 10, exit_from_parent);

    int index = 0;

    while (hd_service_interface_running == 0)
    {
        time_t now = time(NULL);
        HD_LOGGER_INFO(TAG, "%s Main service heartbeat:%d\n", PREFIX, index++);
        sleep(10);
    }

    hd_service_interface_destory();
    HD_LOGGER_INFO(TAG, "%s Main service stopped !!!\n", PREFIX);
    return 0;
}
