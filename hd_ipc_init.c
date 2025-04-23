#include <stdio.h>
#include "hd_ipc_init.h"
#include <jsonrpc-c.h>
#include <stdlib.h>
#include "hd_utils.h"
#include <unistd.h>
#include "hd_logger.h"
#include <pthread.h>
#include <hd_safe_map.h>

#define TAG "hd_ipc_init"

struct HeartbeatTaskData{
    char * name;
    time_t time;
    /** 1:心跳正常 0:心跳断开 */
    int status;
    /** 心跳序号 */
    int index;
    /** 线程 */
    pthread_t pt;

};

static void free_heartbeat_task_data(struct HeartbeatTaskData *data){
    data->status = 0;
    if (data->name)
    {
        free(data->name);
    }
    data->name = NULL;
    data->index=0;
    data->time = 0;
}

static void HeartbeatTaskData_printf(const struct HeartbeatTaskData * data){
    if (data)
    {
        HD_LOGGER_DEBUG(TAG,"<HeartbeatTaskData_printf>pt:%p status:%d index:%d time:%ld \n",&data->pt,data->status,data->index,data->time);
    }
    
}

static  struct jrpc_server my_server;

static ipc_init_on_heartbeat_lost g_ipc_init_heartbeat_lost = NULL;
static ipc_init_on_connected g_ipc_init_on_connected = NULL;
static ipc_callback_core_heartbeat_pong g_callback_heartbeat = NULL;
static ipc_callback_core_service_started g_callback_service_started  = NULL;
static ipc_callback_shell_confirm_upgrade_resp g_callback_upgrade_resp  = NULL;

#define HEART_BEAT_INTERNAL  5

static ThreadSafeMap * g_map = NULL;

static void ipc_init_on_heartbeat_lost_internal(const char * name,int index,time_t time ,int diff);

cJSON*  ipc_request_core_heartbeat_ping(const char * service_name,int index){
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        HD_LOGGER_ERROR(TAG,"<ipc_request_core_heartbeat_ping> fail!\n");
        return NULL;
    }

    cJSON_AddStringToObject(root, "name", service_name);
    cJSON_AddNumberToObject(root, "index", index);
    return root;
}

cJSON*  ipc_request_core_exit_child_execl(const char * service_name){
    return NULL;
}

cJSON*  ipc_request_shell_msg(const char * msg){
    return NULL;
}

cJSON*  ipc_request_shell_error(const char * error,int code){
    return NULL;
}

cJSON*  ipc_request_shell_confirm_upgrade(){
    return NULL;
}

cJSON*  ipc_request_shell_download_progress(const char * service_name,int max ,int progress){
    return NULL;
}


static cJSON * ipc_resp_cmd_ipc_core_heartbeat_pong(jrpc_context * ctx, cJSON * params, cJSON *id) {
    if (g_callback_heartbeat)
    {
        // 解析数据
        char name[1024] = {0};
        int index  = 0;
        cJSON * name_item  = cJSON_GetObjectItemCaseSensitive(params,"name");
        if ((cJSON_IsString(name_item)) && (name_item->valuestring != NULL))
        {
            snprintf(name,sizeof(name),"%s",name_item->valuestring);
        }else{
            return NULL;
        }
        
        cJSON * index_item  = cJSON_GetObjectItemCaseSensitive(params,"index");
        if (cJSON_IsNumber(index_item) )
        {
            index = index_item->valueint;
        }else{
            return NULL;
        }

        struct HeartbeatTaskData * data  = NULL;
        void * value = hd_map_get(g_map,name);
        if (value!=NULL)
        {
            data = ( struct HeartbeatTaskData *)value;
            if (data!=NULL)
            {
                if (data->status == 0 )
                {
                    return NULL;
                }else{
                    time_t now = time(NULL);
                    int diff = now -data->time;
                    if(diff > 10){
                        data->status = 0;  
                    } 
                }
                
            }
        }

        if (data != NULL)
        {
                int last_index =  data->index;
                int status = data->status;
                if (last_index + 1 == index)
                {
                    // 更新
                    data->index ++;
                    data->time =  time(NULL);
                    HD_LOGGER_INFO(TAG,"++++++ pong ++++++ %s %d %ld ++++++ pong ++++++ \n\n",data->name,index,data->time);

                    // 继续
                    sleep(HEART_BEAT_INTERNAL);
                    cJSON* result  = g_callback_heartbeat(data->name,last_index+1);
                    
                    return result;
                }else{
                    HD_LOGGER_ERROR(TAG,"hd_ipc_init.c|ipc_resp_cmd_ipc_core_heartbeat_pong|服务:%s index不匹配 %d(new) != %d(old)\n",name,index,last_index);
                }  
        }
            
    
    }
    return NULL;
}

static void * hearbeat_running_thread(void * arg){
    char * service_name = ( char *)arg;
    struct HeartbeatTaskData *old_data = ( struct HeartbeatTaskData *)hd_map_get(g_map,service_name);
    struct HeartbeatTaskData *data = NULL;
    int diff = 0;
    int no_has_this_data = 0;
    while (1)
    {
        sleep(7);
      

        void * value = hd_map_get(g_map,service_name);
        if (value != NULL)
        {
            data = ( struct HeartbeatTaskData *)value;
            if (data!=NULL)
            {
                if (data->status == 0 )
                {
                    break;
                }else{
                    time_t now = time(NULL);
                    diff = now -data->time;
                    if(diff > 10){
                        data->status = 0;  
                        ipc_init_on_heartbeat_lost_internal(service_name,data->index,data->time,diff);
                        break;
                    } 
                }
            }
        }else{
            no_has_this_data = 1;
            break;
        }
    }
    // if (no_has_this_data)
    // {
    //     free(service_name);
    //     return NULL;
    // }
    
    // if (data->pt !=NULL && (&old_data->pt == &(data->pt)))
    // {
    //     ipc_init_on_heartbeat_lost_internal(service_name,data->index,data->time,diff);

    //     // if (data!=NULL)
    //     // {
    //     //     free_heartbeat_task_data(data);
    //     // }
    //     // hd_map_remove(g_map,service_name);
    
    //     HD_LOGGER_ERROR(TAG,">>> 心跳任务结束:%s 一致<<<\n",service_name);
       
    // }else{
    //     HD_LOGGER_ERROR(TAG,">>> 心跳任务结束:%s 不一致 <<<\n",service_name);
    // }
    HD_LOGGER_ERROR(TAG,">>> 心跳任务结束:%s<<<\n",service_name);
    free(service_name);
    return NULL;
}


static int start_heartbeat_task(const char * name,const char * version ,int pid){
    if (name == NULL || version == NULL)
    {
        HD_LOGGER_ERROR(TAG,"<start_heartbeat_task> check fail \n");
        return -1;
    }

    int ret;
    struct HeartbeatTaskData *data  = NULL;

    HD_LOGGER_DEBUG(TAG,"<start_heartbeat_task> remove old ...\n");
    while (1)
    {   
        hd_map_print_all(g_map,"=");
        void * value = hd_map_get(g_map,name);
        if (value != NULL){
            data = ( struct HeartbeatTaskData *)value;
        }else{
            data = NULL;
        }

        if (data!=NULL){
            HD_LOGGER_DEBUG(TAG,">>>> 等待任务:%s删除 ！！！<<<< \n",name);
            data->status = 0;
            pthread_join(data->pt,NULL);
            hd_map_remove(g_map,name);
            free_heartbeat_task_data(data);
            HD_LOGGER_DEBUG(TAG,">>>> 等待任务:%s删除完毕 ！！！<<<< \n",name);
            // sleep(1);
        }else{
            HD_LOGGER_DEBUG(TAG,">>>> 不存在 ！！！<<<< \n");
            break;
        }
    }

    HD_LOGGER_DEBUG(TAG,"<start_heartbeat_task> add new ...\n");
    // 添加新的
    struct HeartbeatTaskData *new_data = malloc(sizeof(struct HeartbeatTaskData));
    if (new_data == NULL) {
        HD_LOGGER_ERROR(TAG,"<start_heartbeat_task> malloc failed");
        return -1;
    }
    new_data->status = 1;
    new_data->index  = 0;
    new_data->time =  time(NULL);
    new_data->name = strdup(name);
    pthread_create(&new_data->pt,NULL,hearbeat_running_thread,strdup(name));
    ret = hd_map_put(g_map,name,new_data);
    if (!ret)
    {
        HD_LOGGER_ERROR(TAG,"<start_heartbeat_task> hd_map_put g_map=%p name=%s data=%p failed \n",g_map,name,new_data);
       return -1;
    }
    HD_LOGGER_DEBUG(TAG,"<start_heartbeat_task> add new ok.\n");
    return  new_data->index;       
    
}

static cJSON * ipc_resp_cmd_ipc_core_service_started(jrpc_context * ctx, cJSON * params, cJSON *id) {
    if(HD_IPC_JSON_DEBUG)
    {
        hd_ipc_print_cjson(params,"<ipc_resp_cmd_ipc_core_service_started>");
    }

    if (g_callback_heartbeat)
    {
        char name[1024] = {0};
        char version[1024] = {0};
        int pid = 0;

        cJSON * name_item  = cJSON_GetObjectItemCaseSensitive(params,"name");
        if ((cJSON_IsString(name_item)) && (name_item->valuestring != NULL))
        {
            snprintf(name,sizeof(name),"%s",name_item->valuestring);
        }else{
            return NULL;
        }
        cJSON * version_item  = cJSON_GetObjectItemCaseSensitive(params,"version");
        if ((cJSON_IsString(version_item)) && (version_item->valuestring != NULL))
        {
            snprintf(version,sizeof(version),"%s",version_item->valuestring);
        }else{
            return NULL;
        }
        cJSON * pid_item  = cJSON_GetObjectItemCaseSensitive(params,"pid");
        if ((cJSON_IsNumber(pid_item)) )
        {
           pid = pid_item->valueint;
        }else{
            return NULL;
        }
        // 启动心跳任务
        HD_LOGGER_DEBUG(TAG,"start_heartbeat_task %s %s %d ...... \n",name,version,pid);
        int index  = start_heartbeat_task(name,version,pid);
        HD_LOGGER_DEBUG(TAG,"start_heartbeat_task %s %s %d result:%d \n",name,version,pid,index);
        if (index<0)
        {
            return NULL;
        }
        
        if (g_ipc_init_on_connected)
        {
            g_ipc_init_on_connected(name,pid,version);
        }
        
        HD_LOGGER_INFO(TAG,"+++ 服务[%s](%s)-%d已启动！ +++\n\n",name,version,pid);
        return ipc_request_core_heartbeat_ping(name,index);
    }
    return NULL;
}

static cJSON * ipc_resp_cmd_ipc_shell_confirm_upgrade_resp(jrpc_context * ctx, cJSON * params, cJSON *id) {
    if (g_callback_upgrade_resp)
    {
       return g_callback_upgrade_resp("hamain");
    }
    return cJSON_CreateString("ipc_resp_cmd_ipc_shell_confirm_upgrade_resp!");
}

int  ipc_init_send(const cJSON * data){
    //...
    return 0;
}

static void ipc_init_on_heartbeat_lost_internal(const char * name,int index,time_t time,int diff){
    HD_LOGGER_ERROR(TAG,"%s timeout !!!!!! index=%d time=%ld diff=%d\n",name,index,time,diff);
    if (g_ipc_init_heartbeat_lost)
    {
        g_ipc_init_heartbeat_lost(name,index,time);
    }
    
}

int ipc_init_initialize(
    ipc_init_on_connected call_ipc_init_on_connected,
    ipc_init_on_heartbeat_lost callback_ipc_on_init_heartbeat_lost,
    ipc_callback_core_heartbeat_pong callback_heartbeat,
    ipc_callback_core_service_started callback_service_started,
    ipc_callback_shell_confirm_upgrade_resp callback_upgrade_resp
){
    HD_LOGGER_INFO(TAG,"<ipc_init_initialize>");
    g_map = hd_map_init(20);
    g_ipc_init_on_connected = call_ipc_init_on_connected;
    g_ipc_init_heartbeat_lost = callback_ipc_on_init_heartbeat_lost;
    g_callback_heartbeat = callback_heartbeat;
    g_callback_service_started = callback_service_started;
    g_callback_upgrade_resp = callback_upgrade_resp;
    jrpc_server_init(&my_server, PORT_INIT);
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_core_heartbeat_pong, CMD_ipc_core_heartbeat_pong, NULL );
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_core_service_started, CMD_ipc_core_service_started, NULL );
    jrpc_register_procedure(&my_server, ipc_resp_cmd_ipc_shell_confirm_upgrade_resp, CMD_ipc_shell_confirm_upgrade_resp, NULL );
    jrpc_server_run(&my_server);
    return 0;
}




static void cancelHeartbeatThread(void* value, void* context){
    // struct HeartbeatTaskData *data = ( struct HeartbeatTaskData *)value;
    // if (data==NULL)
    // {
    //     return ;
    // }
    // HD_LOGGER_DEBUG(TAG,"<cancelHeartbeatThread> pthread_join->%s ...\n",data->name);
    // data->status = 0;
    // pthread_join(data->pt,NULL);
    // if (data->name)
    // {
    //     free(data->name);
    // }
    // data->name = NULL;
    // data->index=0;
    // pthread_attr_destroy(&data->pt);
    // data->time = 0;
}

static void cancelHeartbeatThreads(){
    hd_map_for_each(g_map,cancelHeartbeatThread,NULL);
}

void ipc_init_destory(){
    HD_LOGGER_INFO(TAG,"<ipc_init_destory>");
    g_ipc_init_on_connected = NULL;
    g_callback_heartbeat = NULL;
    g_callback_service_started = NULL;
    g_callback_upgrade_resp = NULL;
    g_ipc_init_heartbeat_lost = NULL;
    hd_map_deinit(g_map);
    g_map = NULL;
    jrpc_server_destroy(&my_server);
  
}



