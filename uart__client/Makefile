TARGET=uart_client
CC=arm-buildroot-linux-gnueabihf-gcc

LIBEV_DIR=/home/book/source/libev/libev_pc/tmp
JSONRPC_DIR=/home/book/source/jsonrpc/jsonrpc-c_pc/tmp

CFLAGS += -I${LIBEV_DIR}/include -I${JSONRPC_DIR}/include
LDFLAGS += -L${JSONRPC_DIR}/lib -L${LIBEV_DIR}/lib

CFLAGS += -I/home/book/proj/hdinit/includes
LDFLAGS += -L/home/book/proj/hdinit/libs

LDFLAGS += -lev -ljsonrpcc -lpthread -lhdservice
		
c_files = ../cJSON.c ../mqtt__client/rpc_client.c uart_client.c

all:
	${CC} ${CFLAGS} -o ${TARGET} ${c_files} ${LDFLAGS}

clean:
	rm -f *.o ${TARGET}
	
