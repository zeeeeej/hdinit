#include "../hd_service.h"

int main() {
    HDServiceArray sa;
    hd_service_array_init(&sa);

    HDService svc1 = {
        .name = "network",
        .path = "/usr/bin/networkd",
        .is_main = 1,
        .depends_on_count = 1,
        .depends_on = {"logging"}
    };

    HDService svc2 = {
        .name = "logging",
        .path = "/usr/bin/loggingd",
        .is_main = 1
    };

    hd_service_array_add(&sa, &svc2);
    hd_service_array_add(&sa, &svc1);

    hd_service_array_print((const HDServiceArray*)&sa);

    HDService *network = hd_service_array_find_by_name(&sa, "network");
    HDService svc3 = {
        .name = "xxxx",
        .path = "/usr/bin/xxxx",
        .is_main = 1,
        .version = "1.0.0"
    };
    hd_service_array_add(&sa, &svc3);
    hd_service_array_print((const HDServiceArray*)&sa);




    return 0;
}