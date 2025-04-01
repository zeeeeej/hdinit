#include "../hd_service.h"
#include <stdio.h>
/**
 * gcc -o ./test/hd_service_test ./test/hd_service_test.c hd_service.c
 */
int main() {
    HDServiceArray sa;
    hd_service_array_init(&sa);

    HDService svc1 = {
        .name = "network",
        .path = "/usr/bin/networkd",
        .depends_on_count = 1,
        .depends_on = {"logging"}
    };

    HDService svc2 = {
        .name = "logging",
        .path = "/usr/bin/loggingd",
    };

    hd_service_array_add(&sa, &svc2);
    hd_service_array_add(&sa, &svc1);

    hd_service_array_print((const HDServiceArray*)&sa);

    HDService *network = hd_service_array_find_by_name(&sa, "network");
    HDService svc3 = {
        .name = "xxxx",
        .path = "/usr/bin/xxxx",
        .version = "1.0.0"
    };
    hd_service_array_add(&sa, &svc3);
    hd_service_array_print((const HDServiceArray*)&sa);


    HDService service1  = sa.services[1];
    HDService *service2 = sa.services+1;
    printf(" %p %p %p \n",&service1,service2,&(sa.services[1]));

    printf("%p \n",sa.services+1);
    printf("%p \n",&(sa.services[1]));
    printf("%p \n",&*(sa.services+1));

    HDService *service4 = hd_service_array_find_by_name(&sa,"logging");
    printf("find logging result is %s %s \n",service4->name,service4->path);

    return 0;
}