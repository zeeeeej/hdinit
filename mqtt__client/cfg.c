
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "cfg.h"
#include <jsonrpc-c.h>



/**
 * json 
 *	"
		"uri":"",
		"clientId":"",
		"username":"",
		"password":"",
		"productKey":"",
		"deviceName":""
 	"
 *
 */
int mqtt_read_cfg(
	char *uri,
	char *clientId,
	char * username,
	char *password,
	char *productKey,
	char *deviceName
){
	char buf[1000];
	int fd = open("/etc/mqtt.cfg",O_RDONLY);
	if (fd<0)
	{
		printf("mqtt_read_cfg open fail . path : %s",MQTT_CFG_FILE);
		return -1;
	}
	int ret = read(fd,buf,sizeof(buf));
	if (ret<=0)
	{
		printf("mqtt_read_cfg read fail . path : %s",MQTT_CFG_FILE);
		return -1;
	}

	// 解析Json
	cJSON * root = cJSON_Parse(buf);
	cJSON *pt_tmp;
	pt_tmp = cJSON_GetObjectItem(root,"uri");
	if(pt_tmp)
	{
		strcpy(uri,pt_tmp->valuestring);
	}
	pt_tmp = cJSON_GetObjectItem(root,"clientId");
	if(pt_tmp)
	{
		strcpy(clientId,pt_tmp->valuestring);
	}
	pt_tmp = cJSON_GetObjectItem(root,"username");
	if(pt_tmp)
	{
		strcpy(username,pt_tmp->valuestring);
	}
	pt_tmp = cJSON_GetObjectItem(root,"password");
	if(pt_tmp)
	{
		strcpy(password,pt_tmp->valuestring);
	}
	pt_tmp = cJSON_GetObjectItem(root,"productKey");
	if(pt_tmp)
	{
		strcpy(productKey,pt_tmp->valuestring);
	}
	pt_tmp = cJSON_GetObjectItem(root,"deviceName");
	if(pt_tmp)
	{
		strcpy(deviceName,pt_tmp->valuestring);
	}
	cJSON_Delete(root);

	return 0;
}
