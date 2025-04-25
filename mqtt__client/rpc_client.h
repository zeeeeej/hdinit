#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H


int rpc_led_control( int on);

int rpc_dht11_read(int *humi,int *temp);

int rpc_led_read(int *led);

int RPC_Client_Init(void) ;

#endif // RPC_CLIENT_H
