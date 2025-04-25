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
#include "cfg.h"
#include <MQTTClient.h>
#include "rpc_client.h"
#include "cJSON.h"
#include <hd_ipc_service.h>

#define QOS        		0
#define TOPIC_UP 		"/iot/up"
#define TOPIC_DOWN 		"/iot/down"
#define CA_CERTIFICATE_FILE_Path "/etc/mqtt.crt"
#define FREQ_UP 		2


int cache_temp;
int cache_humi;
int cache_led;

/* MQTT START*/
static MQTTClient client;
static volatile MQTTClient_deliveryToken deliveredtoken;

MQTTClient_SSLOptions configureSSLOptions() {
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    ssl_opts.enableServerCertAuth = 1;
    // CA_CERTIFICATE_FILE_Path specify server CA
     ssl_opts.trustStore = CA_CERTIFICATE_FILE_Path;
     //ssl_opts.verify=0;
    return ssl_opts;
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("[mqtt]Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	printf("[mqtt]mqtt msgarrvd !\n");
	int rc;
    int i;
    char* payloadptr;
    printf("[mqtt]Message arrived\n");
    printf("[mqtt]     topic: %s\n", topicName);
    printf("[mqtt]   message: ");
    payloadptr = message->payload;
    /* 解析控制设备 */
	/* 消息格式：{"cmd":"set","params":{"led0":1}} */
	cJSON *root = cJSON_Parse(payloadptr);
	cJSON *cmd = cJSON_GetObjectItem(root,"cmd");
	cJSON *params = cJSON_GetObjectItem(root,"params");
	if(params){
		cJSON *led0 = cJSON_GetObjectItem(params,"led0");
		if(led0){
			rpc_led_control(led0->valueint);
		}
	}
	cJSON_Delete(root);
	
	
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
void connlost(void *context, char *cause)
{
    printf("[mqtt]\nConnection lost\n");
    printf("[mqtt]     cause: %s\n", cause);
}

void*publish_thread(void*arg){
	char buf[1000];

	while(1){

		int humi ;
		int temp ;
		int led0 ;
		while(0!=rpc_dht11_read(&humi,&temp));
		while(0!=rpc_led_read(&led0));
		/* read */
		
		if(cache_humi !=humi || cache_temp != temp || cache_led != led0){
				cache_humi = humi;
				cache_temp = temp;
				cache_led = led0;

				printf("[mqtt]read for publish -> humi:%d,temp:%d,led:%d\n",humi,temp,led0);
				
				MQTTClient_message pubmsg = MQTTClient_message_initializer;
				char * PAYLOAD = "{\
						\"cmd\":\"report\",\
						\"params\":{\
							\"humi\":\"%d\",\
							\"temp\":\"%d\",\
							\"led0\":\"%d\"\
						}\
						}\
					";
				sprintf(buf,PAYLOAD,humi,temp,led0);
				pubmsg.payload = buf;
				pubmsg.payloadlen = strlen(buf);
				pubmsg.qos = QOS;
				pubmsg.retained = 0;
				MQTTClient_publishMessage(client, TOPIC_UP, &pubmsg,(MQTTClient_deliveryToken*) &deliveredtoken);
		}
		sleep(FREQ_UP);
	}
	return NULL;
}

void mqtt_init();

void* mqtt_init_thread(void * arg){
	mqtt_init();
	return NULL;
}

void mqtt_init(){

	printf("[mqtt]mqtt_init\n");
	/* 1.读取mqtt配置文件 */
	char uri[1000];
	char clientId[1000];
	char username[1000];
	char password[1000];
	char productKey[1000];
	char deviceName[1000];
	int index=0;
	while(index<5){
		if(0!=mqtt_read_cfg( uri,  clientId, username,  password,  productKey,  deviceName)){
			printf("[mqtt]mqtt_read_cfg fail\n");
			sleep(1);
			index++;
		}else{
			break;	
		}
	}
	/* 链接mqtt */
	int rc;
	MQTTClient_create(&client, uri, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
 	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.username = username;
	conn_opts.password = password;
	//conn_opts.MQTTVersion = MQTTVERSION_5;
	conn_opts.MQTTVersion = MQTTVERSION_3_1_1;
	printf("[mqtt]mqtt version :%d\n",conn_opts.MQTTVersion);
	MQTTClient_SSLOptions ssl_opts = configureSSLOptions();
	conn_opts.ssl = &ssl_opts;
	// 
	while(1){
		if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    			printf("[mqtt]Failed to connect, return code %d\n", rc);
		//	exit(-1);
			sleep(1);
		} else {
   	 		printf("[mqtt]Connected to MQTT Broker!\n");
			break;
		}
	}
	
	/* create publish thread. */
	/* 读取led 温湿度信息，发布到broker	*/
	pthread_t thread_id;
	int ret = pthread_create(&thread_id,NULL,publish_thread,NULL);
	//printf("create publish thread ret = %d\n",ret);
	/* 订阅消息 控制设备 */
	printf("Subscribing to topic %s for client %s using QoS%d\n\n", TOPIC_DOWN, clientId, QOS);
        MQTTClient_subscribe(client, TOPIC_DOWN, QOS);
	
	MQTTClient_waitForCompletion(client,deliveredtoken,10000);
   	pthread_join(thread_id,NULL);
	printf("[mqtt]>>>>>>>>>>>>>>>>>>>>>>>> mqtt_init end!!!!!!!!\n");
	MQTTClient_disconnect(client, 10000);
   	MQTTClient_destroy(&client);
	
}

/* MQTT END*/


int main(int argc, char **argv)
{
	printf("[mqtt]rpc_client init.\n");
	ipc_service_init("hdmqtt",getpid(),"0.0.1",NULL,NULL);
	RPC_Client_Init();
	pthread_t mqtt_init_t;
	pthread_create(&mqtt_init_t,NULL,mqtt_init_thread,NULL);
	pthread_join(mqtt_init_t,NULL);
	ipc_service_destory();
	return 0;
}


