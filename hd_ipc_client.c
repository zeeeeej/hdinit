#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "hd_ipc_client.h"
#include "hd_logger.h"
#include "hd_utils.h"
#include "hd_ipc_protocol.h"


static ipc_client_recv_func g_ipc_client_recv_func = NULL;

static int g_iSocketClient = 0;

#define BUFF_SIZE 2048

int ipc_client_send(cJSON* data){
    if(HD_IPC_JSON_DEBUG == 1)
    {
        hd_ipc_print_cjson(data,"<ipc_client_send>");
    }
    if (g_iSocketClient)
    {
        char buf[BUFF_SIZE] = {0};
        size_t len;
        int ret = -1;

        char *unformatted_string = cJSON_PrintUnformatted(data);
        sprintf(buf, "%s", unformatted_string);

        len = send(g_iSocketClient, buf, strlen(buf), 0);
        if (len == strlen(buf))
        {
            free(unformatted_string);
            return 0;
        }
        else
        {
            HD_PRINT_ERROR("hd_ipc_client.c","ipc_client_send","send fail %zu \n",len);
            free(unformatted_string);
            return -1;
        }
    }
    return -1;
     
}

void ipc_client_recv(ipc_client_recv_func func){
    g_ipc_client_recv_func = func;
    char buf[BUFF_SIZE] = {0};
    size_t iLen;
    while (1) 
    {
        if (!g_iSocketClient)
        {
            break;
        }
        
        iLen = read(g_iSocketClient, buf, sizeof(buf));
        buf[iLen] = '\0';
        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                if (g_ipc_client_recv_func)
                {
                    g_ipc_client_recv_func(result);
                }
                
                cJSON_Delete(root);
            }
            else
            {
                cJSON_Delete(root);
            }
        }
        else
        {
            HD_PRINT_ERROR("hd_ipc_client.c","ipc_client_recv","read fail %zu \n",iLen);
        }
    }
    printf("[rpc-ipc_client_recv] exit !!! \n");
}

int ipc_client_initialize(const char * server_addr,int server_port){
    struct sockaddr_in tSocketServerAddr;
    int iRet;

    g_iSocketClient = socket(AF_INET, SOCK_STREAM, 0);

    tSocketServerAddr.sin_family      = AF_INET;
    tSocketServerAddr.sin_port        = htons(server_port);  /* host to net, short */
    //tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
    inet_aton(server_addr, &tSocketServerAddr.sin_addr);
    memset(tSocketServerAddr.sin_zero, 0, 8);

    iRet = connect(g_iSocketClient, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        printf("[rpc-client]connect error!\n");
        return -1;
    }
    return 0;
}

void ipc_client_destory(){
    if (g_iSocketClient)
    {
        close(g_iSocketClient);
        g_iSocketClient = 0;
    }
    
    g_ipc_client_recv_func = NULL;
}



