#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "cJSON.h"

#define PORT  1234

static int g_iSocketClient;

int rpc_led_control( int on)
{
	printf("[rpc-client]rpc_led_control %d\n",on);
    char buf[100];
    size_t iLen;
    int ret = -1;

    sprintf(buf, "{\"method\": \"led_control\", \"params\": [%d], \"id\": \"2\" }", on);
    iLen = send(g_iSocketClient, buf, strlen(buf), 0);
    if (iLen ==  strlen(buf))
    {
        while (1) 
        {
            iLen = read(g_iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        } 
        
        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            ret = result->valueint;
            cJSON_Delete(root);
            return ret;
        }
        else
        {
            printf("[rpc-client]rpc_led_control reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("[rpc-client]send rpc_led_control err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

int rpc_dht11_read(int *humi, int *temp)
{
    char buf[300];
    size_t iLen;

    sprintf(buf, "{\"method\": \"dht11_read\"," \
                   "\"params\": [0], \"id\": \"2\" }");        
            
    iLen = send(g_iSocketClient, buf, strlen(buf), 0);
    if (iLen ==  strlen(buf))
    {
        while (1) 
        {
            iLen = read(g_iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        } 
        
        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                cJSON * a = cJSON_GetArrayItem(result,0);
                cJSON * b = cJSON_GetArrayItem(result,1);
                //printf("[rpc-client]rpc_dht11_read  : %d,%d\n", a->valueint,b->valueint);
                *humi = a->valueint;//static_cast<unsigned char>(a->valueint);
                *temp = b->valueint;//static_cast<unsigned char>(b->valueint);
                
                cJSON_Delete(root);
                return 0;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            printf("[rpc-client]rpc_dht11_read err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("[rpc-client]send rpc_dht11_read err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

int rpc_led_read(int *led){
    char buf[300];
    size_t iLen;

    sprintf(buf, "{\"method\": \"led_read\"," \
                   "\"params\": [0], \"id\": \"2\" }");

    iLen = send(g_iSocketClient, buf, strlen(buf), 0);
    if (iLen ==  strlen(buf))
    {
        while (1)
        {
            iLen = read(g_iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        }

        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                *led = result->valueint;
                //printf("[rpc-client]rpc_led_read  : %d\n", result->valueint);
                cJSON_Delete(root);
                return 0;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            printf("[rpc-client]rpc_led_read err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("[rpc-client]send rpc_led_read err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

/* 连接RPC Server
 * 返回值: (>0)socket, (-1)失败
 */
void RPC_Client_Init(void)
{

    struct sockaddr_in tSocketServerAddr;
    int iRet;

    g_iSocketClient = socket(AF_INET, SOCK_STREAM, 0);

    tSocketServerAddr.sin_family      = AF_INET;
    tSocketServerAddr.sin_port        = htons(PORT);  /* host to net, short */
    //tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
    inet_aton("127.0.0.1", &tSocketServerAddr.sin_addr);
    memset(tSocketServerAddr.sin_zero, 0, 8);


    iRet = connect(g_iSocketClient, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        printf("[rpc-client]connect error!\n");
        return ;
    }
}
