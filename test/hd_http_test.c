#include "../hd_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../hd_utils.h"

void progress_callback(int *progress) {
    printf("Download progress: %d%%\n", *progress);
}

/**
 * gcc -o ./test/hd_http_test ./test/hd_http_test.c hd_http.c ./cJSON.c -lcurl hd_utils.c
 */
int main() {
    hd_http_init();
    const char * service_name = "hdmain";
    char test_api_url[1024] ;
    /* case 01 (int)hd_http_check_update_url(const char *, char *); */
    printf("<case 01> hd_http_check_update_url\n");
    if (hd_http_check_update_url(service_name, test_api_url)) {
        printf("hd_http_check_update_url-> test_api_url=%s \n",test_api_url);
        return -1;
    }
    printf("hd_http_check_update_url-> test_api_url=%s \n",test_api_url);

    char buff[1024];
    snprintf(buff, 1024, "%s%s/%s", HOST, API_CHECK_UPDATE, service_name);

    if (strcmp(buff,test_api_url)!=0)
    {
        printf("hd_http_check_update_url测试失败!\n");
        EXIT_FAILURE;
    }


    /* case 02 (int)hd_http_check_update(const char *, char *); */
    printf("<case 02> hd_http_check_update\n");
    hd_http_check_resp resp;
    
    if (hd_http_check_update("hdmain", &resp) == 0) {
        printf("Update available:\n");
        printf("URL: %s\n", resp.url);
        printf("Version: %s\n", resp.version);
        printf("MD5: %s\n", resp.md5);
        
        /* case 03 (int)hd_http_check_update(const char *, char *); */
        printf("<case 03> hd_http_download\n");
        
        snprintf(buff, 1024, "./test/ota/%s", resp.filename);
        hd_http_download(resp.url, buff, progress_callback);

        // cp
        hd_cp_file(buff,"./test/ota/hdmain-cp");
    
    } else {
        printf("No update available\n");
    }
    sync();
    
    return 0;
}