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
#include "hd_service_interface.h"


#define TAG "hdlog"
#define PREFIX ">>>>>>"
#define VERSION_LOG "1.0.2"

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


/**
 * gcc hd_log.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c hd_service_interface.c -o ./.service/hdlog
 * gcc  hd_log.c hd_logger.c hd_utils.c hd_ipc.c cJSON.c -o ./server/files/hdlog-1.0.2
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
    HD_LOGGER_INFO(TAG,"%s Log service started (PID: %d)\n",PREFIX, getpid());
    hd_service_interface_init(argv[1],"hdlog",VERSION_LOG,5,exit_from_parent);
    int index = 0;
    HD_LOGGER_INFO(TAG,"%s Log service heartbeat start ... ... ... <%d>\n",PREFIX,hd_service_interface_running);
    while (hd_service_interface_running==0) {
        HD_LOGGER_INFO(TAG,"%s Log service heartbeat:%d <%d> \n",PREFIX, index++,hd_service_interface_running);
       
        time_t now = time(NULL);
  
        sleep(3);
    }
    hd_service_interface_destory();
    HD_LOGGER_INFO(TAG,"%s Log service heartbeat stop!!!!<%d>\n",PREFIX,hd_service_interface_running);
    return 0;
}
