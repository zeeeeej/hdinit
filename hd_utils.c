#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "hd_utils.h"




#define BUFFER_SIZE 4096

time_t hd_get_last_modified(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}

int hd_check_argc_argv_has_debug(int argc,  const char *argv[]){
    if (argc<=1)
    {
        return 0;
    } 
    for(int i =0 ;i<argc;i++){
        if (0==strcmp(DEBUG_PARAM,(argv[i])))
        {
            return 0;
        }
        
    }
    return -1;
    
}

int hd_printf_argc_argv(int argc,char const *argv[],char * const result){
    char temp[2048];
    result[0] = '\0'; // 清空结果字符串

    // 添加标题行
    snprintf(temp, sizeof(temp), "\nargc=%d \n", argc);
    strcat(result, temp);
    for(int i = 0;i<argc;i++){
        snprintf(temp, sizeof(temp), "  argv[%d] %s \n", i,argv[i]);
        strcat(result, temp);
    }
    snprintf(temp, sizeof(temp), "\n");
    return 0;
}


// 递归创建目录（等效于 mkdir -p）
static int hd_mkdir_p(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) return -1;

    char *current = path_copy;
    char *slash = current;

    // 处理相对路径（如 ./ 或 ../）
    if (*current == '/') current++; // 绝对路径跳过首字符
    else if (strncmp(current, "./", 2) == 0) current += 2;
    else if (strncmp(current, "../", 3) == 0) current += 3;

    while ((slash = strchr(current, '/')) != NULL) {
        *slash = '\0'; // 临时截断路径
        struct stat st;
        if (stat(path_copy, &st) == -1) {
            if (mkdir(path_copy, 0755) == -1 && errno != EEXIST) {
                fprintf(stderr, "Failed to create %s: %s\n", path_copy, strerror(errno));
                free(path_copy);
                return -1;
            }
            printf("Created directory: %s\n", path_copy);
        } else if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s exists but is not a directory!\n", path_copy);
            free(path_copy);
            return -1;
        }
        *slash = '/';
        current = slash + 1;
    }

    // 创建最终目录
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create %s: %s\n", path, strerror(errno));
        free(path_copy);
        return -1;
    }

    free(path_copy);
    return 0;
}

// 检查并创建文件路径
int hd_if_file_path_not_exists_create_path(const char *file_path) {
    char *dir_path = strdup(file_path);
    if (!dir_path) return -1;

    char *path = dirname(dir_path); // 提取目录部分
    int ret = hd_mkdir_p(path);     // 递归创建目录
    free(dir_path);
    return ret;
}

// 创建目录（如果不存在）
static int hd_if_path_not_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        // 目录不存在，尝试创建
        if (mkdir(path, 0755) == -1) {
            fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
            return -1;
        }
        printf("Created directory: %s\n", path);
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s exists but is not a directory!\n", path);
        return -1;
    }
    return 0;
}

const char *hd_get_filename(const char *path) {
    const char *filename = strrchr(path, '/'); // 查找最后一个 '/'
    return filename ? filename + 1 : path;     // 如果找到返回后续部分，否则返回原路径
}

// 备份文件
int hd_cp_file(const char *src_path, const char *dst_path) {
    int src_fd, dst_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    struct stat st;

    // 检查源文件是否存在
    if (stat(src_path, &st) == -1) {
        fprintf(stderr, "Source file %s does not exist: %s\n", src_path, strerror(errno));
        return -1;
    }

    // 打开源文件
    if ((src_fd = open(src_path, O_RDONLY)) == -1) {
        fprintf(stderr, "Error opening source file %s: %s\n", src_path, strerror(errno));
        return -1;
    }

    // 创建目标目录（如果不存在）
    char *dir_path = strdup(dst_path); // 复制路径字符串
    char *dir = dirname(dir_path);     // 提取目录部分
    if (hd_if_path_not_exists(dir) == -1) {
        free(dir_path);
        close(src_fd);
        return -1;
    }
    free(dir_path);

    // 打开目标文件（如果存在则覆盖）
    if ((dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode)) == -1) {
        fprintf(stderr, "Error opening destination file %s: %s\n", dst_path, strerror(errno));
        close(src_fd);
        return -1;
    }

    // 复制文件内容
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
            close(src_fd);
            close(dst_fd);
            return -1;
        }
    }

    if (bytes_read == -1) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    // 关闭文件
    close(src_fd);
    close(dst_fd);

    printf("Successfully backed up %s to %s\n", src_path, dst_path);
    return 0;
}
