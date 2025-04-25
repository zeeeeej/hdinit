#ifndef _CFG_H
#define _CFG_H

#define MQTT_CFG_FILE "/ect/mqtt.cfg"

int mqtt_read_cfg(
	char *uri,
	char *clientId,
	char * username,
	char *password,
	char *productKey,
	char *deviceName
);

#endif