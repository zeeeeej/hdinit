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
#include <pthread.h>
#include "dht11.h"
#include "led.h"
#include "cJSON.h"
#include <jsonrpc-c.h>
#include <signal.h>


volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

static struct jrpc_server my_server;
#define PORT 			1234
#define VERSION 		8
// properties cache
static volatile int cache_humi = -1;
static volatile int cache_temp = -1;
static volatile char cache_led = -1;
// properties cache end

/*
 * rpc_led_control
 * params:{"params":[0|1]}
 */
cJSON * server_led_control(jrpc_context * ctx, cJSON * params, cJSON *id) {
    cJSON *status = cJSON_GetArrayItem(params, 0);
    printf("[rpc]server_led_control %d =>version:%d",status,VERSION);
    led_control(status->valueint);
    cache_led = status->valueint;
    return cJSON_CreateNumber(0);
}

/*
 * rpc_dht11_read
 * return:{"params":[humi,temp]}
 */
cJSON * server_dht11_read(jrpc_context * ctx, cJSON * params, cJSON *id) {
    int array[2] = {(int)cache_humi,(int)cache_temp};
    return cJSON_CreateIntArray(array,2);
}

/**
 * rpc_led_read
 * return :{"params":1}
 *
 */

cJSON * server_led_read(jrpc_context * ctx, cJSON * params, cJSON *id) {
    return cJSON_CreateNumber((int)cache_led);
}
/**
 * rpc_heartbeat
 * return :{"params":"ping"}
 *
 */
#define PONG 0
cJSON * server_heartbeat(jrpc_context * ctx, cJSON * params, cJSON *id) {
    return cJSON_CreateNumber(PONG);
}


/**
 * thread for update properties
 *
 */
void * dht11_read_thread(void * arg){
    int humi;
    int temp;
    char led;
    while(1)
    {
    	while(0!=dht11_read(&humi,&temp)){
    		sleep(1);
   	}
   	cache_humi = (int)humi;
    	cache_temp = (int)temp;
	while(0!=led_read(&led)){
		sleep(1);
	}
	cache_led = led;   
    }
    return NULL;
}


int RPC_Server_Init(void) 
{
    printf("[rpc]rpc_server_init.version:%d. \n",VERSION);
    int err;
    err = jrpc_server_init(&my_server, PORT);
    if (err)
    {
        printf("[rpc]jrpc_server_init err : %d\n", err);
    }
    
    jrpc_register_procedure(&my_server, server_led_control, "led_control", NULL );
    jrpc_register_procedure(&my_server, server_dht11_read, "dht11_read", NULL );
    jrpc_register_procedure(&my_server, server_led_read, "led_read", NULL );
    jrpc_register_procedure(&my_server, server_heartbeat, "heartbeat", NULL );
	
    pthread_t threadId;
    int result = pthread_create(&threadId, NULL, dht11_read_thread, NULL);
    if (result != 0) {
        printf("[rpc]Failed to create thread!\n");
        return 1;
    }    

    while(0!=     led_control(0));
    cache_led =0;
    
    jrpc_server_run(&my_server);
    jrpc_server_destroy(&my_server);

    return 0;
}

int main() {
    printf("hdmain service started (PID: %d)\n", getpid());
    
    signal(SIGTERM, handle_signal);
    
    // 主业务逻辑
 	printf("[rpc]led_init.\n");
	led_init();
	printf("[rpc]dht11_init.\n");
	dht11_init();	
	RPC_Server_Init();
	   
    printf("hdmain service stopped\n");
    return 0;
}
