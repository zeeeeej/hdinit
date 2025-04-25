#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include "../mqtt__client/rpc_client.h"
#include <hd_ipc_service.h>

/* set_opt(fd,115200,8,'N',1) */
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	
	if ( tcgetattr( fd,&oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}
	
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/

	switch( nBits )
	{
	case 7:
		newtio.c_cflag |= CS7;
	break;
	case 8:
		newtio.c_cflag |= CS8;
	break;
	}

	switch( nEvent )
	{
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
	break;
	case 'E': 
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
	break;
	case 'N': 
		newtio.c_cflag &= ~PARENB;
	break;
	}

	switch( nSpeed )
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
	break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
	break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
	break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	}
	
	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB;
	
	newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
	newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间: 
	                         * 比如VMIN设为10表示至少读到10个数据才返回,
	                         * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
	                         * 假设VTIME=1，表示: 
	                         *    10秒内一个数据都没有的话就返回
	                         *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
	                         */

	tcflush(fd,TCIFLUSH);
	
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	//printf("[uart]set done!\n");
	return 0;
}

int open_port(char *com)
{
	int fd;
	//fd = open(com, O_RDWR|O_NOCTTY|O_NDELAY);
	fd = open(com, O_RDWR|O_NOCTTY);
    if (-1 == fd){
		return(-1);
    }
	
	  if(fcntl(fd, F_SETFL, 0)<0) /* 设置串口为阻塞状态*/
	  {
			printf("[uart]fcntl failed!\n");
			return -1;
	  }
  
	  return fd;
}

//////***////////

void print_buffer_hex(const u_int8_t *buf, size_t len) {
    printf("[uart]");
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buf[i]); // 以 16 进制形式打印每个字节
    }
    printf("\n");
}

#define FRAME_HEADER 0xCDAB
#define FRAME_FOOTER 0xBADC

// 计算 CRC 校验（示例，具体实现根据需求）
u_int8_t calculate_crc(const u_int8_t *data, size_t len) {
    u_int8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}
int read_raw_data(int fd, u_int8_t *buf, size_t buf_size) {
    u_int16_t header;
    u_int8_t payload_len;
    u_int8_t crc;
    u_int16_t footer;

    // 读取帧头
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        printf("[uart]Failed to read header\n");
        return -1; // 读取失败
    }

    printf("[uart]Header: 0x%04X\n", header);

    if (header != FRAME_HEADER) {
        printf("[uart]Invalid header: 0x%04X\n", header);
        return -1; // 帧头不匹配
    }

    // 读取 payload 长度
    if (read(fd, &payload_len, sizeof(payload_len)) != sizeof(payload_len)) {
        printf("[uart]Failed to read payload length\n");
        return -1; // 读取失败
    }

    printf("[uart]Payload length: %d\n", payload_len);

    if (payload_len > buf_size) {
        printf("[uart]Payload length exceeds buffer size\n");
        return -1; // payload 长度超出缓冲区大小
    }

    // 读取 payload
    if (read(fd, buf, payload_len) != payload_len) {
        printf("[uart]Failed to read payload\n");
        return -1; // 读取失败
    }

    // 读取 CRC
    if (read(fd, &crc, sizeof(crc)) != sizeof(crc)) {
        printf("[uart]Failed to read CRC\n");
        return -1; // 读取失败
    }

    // 读取帧尾
    if (read(fd, &footer, sizeof(footer)) != sizeof(footer)) {
        printf("[uart]Failed to read footer\n");
        return -1; // 读取失败
    }

    printf("[uart]Footer: 0x%04X\n", footer);

    if (footer != FRAME_FOOTER) {
        printf("[uart]Invalid footer: 0x%04X\n", footer);
        return -1; // 帧尾不匹配
    }

    // 校验 CRC
    if (calculate_crc(buf, payload_len) != crc) {
        printf("[uart]warning:CRC check failed\n");
       // return -1; // CRC 校验失败
    }

    return payload_len; // 返回 payload 长度
}

void parse_payload(const u_int8_t *payload, size_t len) {
    if (len < 2) {
        printf("[uart]Invalid payload length\n");
        return;
    }

    u_int8_t direction = payload[0];
    u_int8_t cmd = payload[1];

    if (direction == 0x01) { // write app->device
        if (cmd == 0x01) { // set led
            if (len < 3) {
                printf("[uart]Invalid payload length for set led command\n");
                return;
            }
            u_int8_t onoff = payload[2];

            printf("[uart]Set LED: %s\n", onoff ? "ON" : "OFF");
	    if(onoff){
	    	rpc_led_control(1);
	    }else{
		rpc_led_control(0);
	    }
        } else {
            printf("[uart]Unknown command: 0x%02X\n", cmd);
        }
    } else if (direction == 0x02) { // notify device->app
        if (cmd == 0x02) { // notify data
            if (len < 5) {
                printf("[uart]Invalid payload length for notify data command\n");
                return;
            }
            u_int8_t led = payload[2];
            u_int8_t humi = payload[3];
            u_int8_t temp = payload[4];
	   // write
            printf("[uart]Only For Mock Notify Data: LED=%s, Humidity=%d, Temperature=%d\n", led ? "ON" : "OFF", humi, temp);
        } else {
            printf("[uart]Unknown command: 0x%02X\n", cmd);
        }
    } else {
        printf("[uart]Unknown direction: 0x%02X\n", direction);
    }
}

static int fd;

void * notify_thread_function(void *arg){
	int size = 11;
	char buf[size];

	while(1){
		if(fd){
		int humi ;
		int temp ;
		int led0 ;
		while(0!=rpc_dht11_read(&humi,&temp));
		while(0!=rpc_led_read(&led0));
		/* read */
		buf[0]=0xAB;
		buf[1]=0xCD;
		buf[2]=0x05;
		buf[3]=0x02;
		buf[4]=0x02;
		buf[5]=led0;
		buf[6]=humi;
		buf[7]=temp;
		buf[8]=0x00;
		buf[9]=0xDC;
		buf[10]=0xBA;

		//buf[11]='\r';
		//buf[12]='\n';


		write(fd,buf,size);

		}
		sleep(1);
	}
	return NULL;
}

/*
 * ./serial_send_recv <dev>
 */
int main(int argc, char **argv)
{
	ipc_service_init("hduart",getpid(),"0.0.1",NULL,NULL);
	int iRet;
	if (argc != 2)
	{
		//printf("[uart]Usage: \n");
		//printf("[uart]%s </dev/ttySAC1 or other>\n", argv[0]);
		//return -1;

		fd = open_port("/dev/ttymxc5");
	} else {

		fd = open_port(argv[1]);
	}

	if (fd < 0)
	{
		printf("[uart]open %s err!\n", argv[1]);
		return -1;
	}

	iRet = set_opt(fd, 9600, 8, 'N', 1);
	if (iRet)
	{
		printf("[uart]set port err!\n");
		return -1;
	}
	printf("[uart]start!\n");
	
	RPC_Client_Init();
	pthread_t notify_thread_t;
	pthread_create(&notify_thread_t,NULL,notify_thread_function,NULL);

	char buf[256];
   	 while (1) {
        	int len = read_raw_data(fd, buf,256);
		if (len > 0) {
	 		print_buffer_hex(buf,len);
            		parse_payload(buf, len);
        	} 
	       	else if (len == -1) {
            		printf("[uart]Failed to read data or invalid data format\n");
       		}
	       	else {
            		printf("[uart]Failed to read data\n");
       		}
    }

	pthread_join(notify_thread_t,NULL);
    close(fd);
	ipc_service_destory();
	return 0;
}

