#ifndef __HD_SERVICE_INTERFACE__
#define __HD_SERVICE_INTERFACE__


typedef struct 
{
    void (*hd_service_init)(void);

    void (*hd_service_on_start)(void);

    void (*hd_service_on_destory)(void);
} hd_service_interface;


#endif // __HD_SERVICE_INTERFACE__