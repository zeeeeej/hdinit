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
#include "hd_utils.h"
#include "hd_ipc.h"

#include "hd_ipc_service.h"

#define TAG "hdlog"
#define PREFIX ">>>>>>"
#define VERSION_LOG "1.0.2"

static int g_running = 1;
static int g_index = 0;

void on_init (){

}

void on_start (){

}

void on_destory (){

}


static void exit_from_parent(){
    HD_LOGGER_INFO(TAG,"%s Log service exit!\n",PREFIX);
    HD_LOGGER_INFO(TAG,"%s Log service exit!\n",PREFIX);
    HD_LOGGER_INFO(TAG,"%s Log service exit!\n",PREFIX);
}

static void   ipc_service_on_exit_internal (){
   g_running = 0;
   exit_from_parent();
}

static void  ipc_service_on_heartbeat_ping_internal (int index){
    g_index = index;
}



/**
 * 
 * export DYLD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/.shared/libhsi.so:$DYLD_LIBRARY_PATH
 * export LD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/.shared/libhsi.so:$LD_LIBRARY_PATH
 * 
 * 
 * 
 * gcc -o ./.service/hdlog hd_log.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c hd_ipc_service.c hd_ipc_client.c hd_ipc_protocol.c -lcurl 
 * 
 * 
 */
int main(int argc,const char *argv[]) {
    if (HD_DEBUG)
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_DEBUG);
    }
    else
    {
        hd_logger_set_level(HD_LOGGER_LEVEL_INFO); 
    }
    HD_LOGGER_INFO(TAG,"%s Log-Service-Started (PID: %d)\n",PREFIX, getpid());
    
    // hd_service_interface_init(argv[1],"hdlog",VERSION_LOG,5,exit_from_parent);

    ipc_service_init("hdlog",getpid(),VERSION_LOG,ipc_service_on_exit_internal,ipc_service_on_heartbeat_ping_internal);

    int index = 0;
    while (g_running) {
        HD_LOGGER_INFO(TAG,"%s Log-Service-Running ... ... ... <%d>-<%d> \n",PREFIX,index++,g_index);
      
        sleep(3);
    }
    // hd_service_interface_destory();
    ipc_service_destory();
 
    HD_LOGGER_INFO(TAG,"%s Log-Service-Start Stopped !!!! \n",PREFIX);
    return 0;
}