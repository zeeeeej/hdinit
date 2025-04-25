TARGET=rpc_server
CC=arm-buildroot-linux-gnueabihf-gcc

LIBEV_DIR=/home/book/source/libev/libev_pc/tmp
JSONRPC_DIR=/home/book/source/jsonrpc/jsonrpc-c_pc/tmp

CFLAGS += -I${LIBEV_DIR}/include -I${JSONRPC_DIR}/include
LDFLAGS += -L${JSONRPC_DIR}/lib -L${LIBEV_DIR}/lib

CFLAGS += -I/home/book/proj/hdinit/includes
LDFLAGS += -L/home/book/proj/hdinit/libs


LDFLAGS += -lm -lpthread -lhdservice -ljsonrpcc -lev

c_files = ../cJSON.c rpc_server.c led.c dht11.c 

all:
	${CC} ${CFLAGS} -o ${TARGET} ${c_files} ${LDFLAGS}

clean:
	rm -f *.o ${TARGET}
	
