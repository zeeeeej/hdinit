#include "hd_ipc_service.h"
#include "hd_ipc_client.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "hd_utils.h"
#include "hd_logger.h"

#define TAG "hd_ipc_service"

static ipc_callback_core_heartbeat_ping g_callback_heartbeat_ping = NULL;
static ipc_callback_core_exit_child_execl g_callback_exit_child_execl = NULL;
static char g_service_name[1024] = {0};  
static int  g_pid = 0;
static char g_version[1024] = {0}; 

static cJSON * resp_CMD_ipc_core_heartbeat_ping(cJSON * params) {
    if (g_callback_heartbeat_ping)
    {
       cJSON * resp  = g_callback_heartbeat_ping("hamain",1);
       return resp;
    }
    return NULL;
}

static cJSON * resp_CMD_ipc_core_exit_child_execl(cJSON * params) {
    if (g_callback_exit_child_execl)
    {
        cJSON * resp  =g_callback_exit_child_execl("hamain");
        return resp;
    }
    return NULL;
}

cJSON* ipc_core_service_started(const char * service_name,int pid,const char * version){
    cJSON *params = cJSON_CreateObject();
    if (!params) {
        HD_LOGGER_ERROR(TAG, "<ipc_core_service_started> cJSON_CreateObject params fail.\n");
        return NULL;
    }
    cJSON_AddStringToObject(params, "name", service_name);
    cJSON_AddNumberToObject(params, "pid", pid);
    cJSON_AddStringToObject(params, "version", version);
    cJSON *root = ipc_client_create_json(CMD_ipc_core_service_started,2,"2.0",params);
    return root;
}

cJSON* ipc_core_heartbeat_pong(const char * service_name,int index){
    cJSON *params = cJSON_CreateObject();
    if (!params) {
        HD_LOGGER_ERROR(TAG, "<ipc_core_heartbeat_pong> cJSON_CreateObject params fail.\n");
        return NULL;
    }
    cJSON_AddStringToObject(params, "name", service_name);
    cJSON_AddNumberToObject(params, "index", index);
    cJSON *root = ipc_client_create_json(CMD_ipc_core_heartbeat_pong,2,"2.0",params);
    return root;
}

static  void  ipc_client_recv_func_internal(cJSON* data){
    char name[1024] = {0};
    int index  = 0;
    cJSON * name_item  = cJSON_GetObjectItemCaseSensitive(data,"name");
    if (cJSON_IsString(name_item) && (name_item->valuestring != NULL))
    {
        snprintf(name,sizeof(name),"%s",name_item->valuestring);
    }else{
        HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> name_item fail.\n");
        return ;
    }
    
    cJSON * index_item  = cJSON_GetObjectItemCaseSensitive(data,"index");
    if (cJSON_IsNumber(index_item) )
    {
        index = index_item->valueint;  
    }else{
        HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> index_item fail.\n");
        return ;
    }

    if (strcmp(g_service_name,name)!=0 )
    {
        HD_LOGGER_ERROR(TAG, "<ipc_client_recv_func_internal> service not same.\n");
       return ;
    }
    
    HD_LOGGER_ERROR(TAG, "------ ping ------ %s %d ------ ping ------\n\n",name,index);

    cJSON * resp_data = ipc_core_heartbeat_pong(name,index+1);
    if (resp_data)
    {
        ipc_client_send(resp_data);
    }
    
}

static void * recv_thread(void * arg){
    ipc_client_recv(ipc_client_recv_func_internal);
    return NULL;
}

int ipc_service_init(
    const char * service_name,
    int pid,
    const char * version,
    ipc_callback_core_heartbeat_ping callback_heartbeat_ping ,
    ipc_callback_core_exit_child_execl callback_exit_child_execl
){
     if (service_name == NULL || version == NULL) {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init> service_name or version error .\n");
        return -1;
    }
   
    int ret ;
    memset(g_service_name, 0, sizeof(g_service_name));
    g_pid = 0;
    memset(g_version, 0, sizeof(g_version));

    strncpy(g_service_name, service_name, sizeof(g_service_name) - 1);
    g_service_name[sizeof(g_service_name) - 1] = '\0'; 

    g_pid = pid;
    
    strncpy(g_version, version, sizeof(g_version) - 1);
    g_version[sizeof(g_version) - 1] = '\0';

    g_callback_exit_child_execl = callback_exit_child_execl;
    g_callback_heartbeat_ping = g_callback_heartbeat_ping;
    ret  = ipc_client_initialize("127.0.0.1",PORT_INIT);
    if (ret!=0)
    {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_client_initialize error %d .\n",ret);
        return  -1;
    }
    
    pthread_t t;
    pthread_create(&t,NULL,recv_thread,NULL);

    HD_LOGGER_INFO(TAG, "<ipc_service_init> %s %d %s\n",service_name,pid,version);

    cJSON * resp_data = ipc_core_service_started(service_name,pid,version);
    if (resp_data)
    {
    ret =  ipc_client_send(resp_data);
       if (ret != 0)
       {
        HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_client_send error %d .\n",ret);
       }else{
        return -1;
       }
       
    }else{
        HD_LOGGER_ERROR(TAG, "<ipc_service_init>ipc_core_service_started error %d .\n",ret);
    }
    //pthread_join(t,NULL);

    return 0;

}

void ipc_service_destory(){
    HD_LOGGER_INFO(TAG, "<ipc_service_destory> %s %d %s\n",g_service_name,g_pid,g_version);
    ipc_client_destory();
    memset(g_service_name, 0, sizeof(g_service_name));
    g_pid = 0;
    memset(g_version, 0, sizeof(g_version));

}




