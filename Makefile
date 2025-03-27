TARGET=hdmain
CC=arm-buildroot-linux-gnueabihf-gcc

LIBEV_DIR=/home/book/Desktop/jsonrpc2/libev_pc/tmp
JSONRPC_DIR=/home/book/Desktop/jsonrpc2/jsonrpc-c_pc/tmp

CFLAGS=-I${LIBEV_DIR}/include -I${JSONRPC_DIR}/include
LDFLAGS=${JSONRPC_DIR}/lib/libjsonrpcc.a  ${LIBEV_DIR}/lib/libev.a -lm -lpthread

c_files = cJSON.c rpc_server.c led.c dht11.c

all:
	${CC} ${CFLAGS}  -o ${TARGET} ${c_files} ${LDFLAGS}

clean:
	rm -f *.o ${TARGET}
	
