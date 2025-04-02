#include <stdio.h>
#include <string.h>
#include "../hd_utils.h"
#include <unistd.h>

#define PATH  "./hdmain2"

/**
 * gcc -o ./test/hd_utils_test ./test/hd_utils_test.c ./hd_utils.c
 */
int main() {

    hd_if_file_path_not_exists_create_path("./test/hdinit/ota-xxx/hdmain-0.0.4");

    const char *source = "./hd_init";
    char buff[1024];
    snprintf(buff,1024,"./bak/%s",hd_get_filename(PATH));
    const char *backup = "./bak/hd_init.bak";

    if (hd_cp_file(source, backup) != 0) {
        fprintf(stderr, "Backup failed\n");
        return 1;
    }

    if (hd_cp_file(source, buff) != 0) {
        fprintf(stderr, "Backup failed\n");
        return 1;
    }
    hd_print_progress_bar(0);
    for (int i = 0; i <= 100; i++) {
        hd_print_progress_bar(i);
        usleep(100000); // 延迟 100ms（微秒）
    }
    printf("\nDone!\n");
    sync();
    return 0;
}