
#ifndef __HD_UTILS__
#define __HD_UTILS__

#include <sys/stat.h>

time_t get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}

#endif // __HD_UTILS__