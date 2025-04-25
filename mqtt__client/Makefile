TARGET=mqtt_client
CC=arm-buildroot-linux-gnueabihf-gcc

LIBEV_DIR=/home/book/source/libev/libev_pc/tmp
JSONRPC_DIR=/home/book/source/jsonrpc/jsonrpc-c_pc/tmp
PAHO_MQTT_DIR=/home/book/paho4imx6ull/paho.mqtt.c/build/output

CFLAGS += -I${LIBEV_DIR}/include -I${JSONRPC_DIR}/include -I${PAHO_MQTT_DIR}/include
LDFLAGS += -L${JSONRPC_DIR}/lib -L${LIBEV_DIR}/lib -L${PAHO_MQTT_DIR}/lib

CFLAGS += -I/home/book/proj/hdinit/includes
LDFLAGS += -L/home/book/proj/hdinit/libs

LDFLAGS +=   -lpaho-mqtt3cs -lssl -lcrypto  -lm -lpthread -lhdservice

c_files = ../cJSON.c cfg.c rpc_client.c mqtt_client.c

all:
	${CC} ${CFLAGS} -o ${TARGET} ${c_files} ${LDFLAGS}

clean:
	rm -f *.o ${TARGET}
	
