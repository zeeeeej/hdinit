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

static cJSON*   p1 (const char * service_name,int index){
    return NULL;
}

static cJSON*   p2 (const char * service_name){
    return NULL;
}



/**
 * 
 * export DYLD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/.shared/libhsi.so:$DYLD_LIBRARY_PATH
 * export LD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/.shared/libhsi.so:$LD_LIBRARY_PATH
 * 
 * 
 * 
 * -> gcc -o ./.service/hdlog hd_log.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c hd_ipc_service.c hd_ipc_client.c   -lcurl 
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

    ipc_service_init("hdlog",getpid(),VERSION_LOG,p1,p2);

    int index = 0;
    while (g_running) {
        HD_LOGGER_INFO(TAG,"%s Log-Service-Running ... ... ... <%d> \n",PREFIX,index++);
       
        time_t now = time(NULL);
  
        sleep(3);
    }
    // hd_service_interface_destory();
    ipc_service_destory();
 
    HD_LOGGER_INFO(TAG,"%s Log-Service-Start Stopped !!!! \n",PREFIX);
    return 0;
}