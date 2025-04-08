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


void hd_print_progress_bar(int percent) {
    const int bar_width = 50; // 进度条宽度（字符数）
    char bar[bar_width + 1];  // +1 用于存储 '\0'
    memset(bar, ' ', bar_width);
    bar[bar_width] = '\0';

    // 计算填充的进度位置
    int filled_len = percent * bar_width / 100;
    for (int i = 0; i < filled_len; i++) {
        bar[i] = '=';
    }
    if (filled_len < bar_width && percent > 0) {
        bar[filled_len] = '>'; // 进度条头部符号
    }

    // 打印进度条（\r 覆盖当前行）
    // printf("\r[%s] %d%%", bar, percent);
    printf("\r\033[K[%s] %d%%", bar, percent); // \033[K 清除右侧所有内容
    printf("\r...");
    fflush(stdout); // 立即刷新输出缓冲区
}


// 线程安全的进度条打印
void hd_print_progress_double(double percentage) {
    const int bar_width = 50;
    int filled = (int)(percentage * bar_width / 100.0);
    
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
    }
    printf("] %.1f%%", percentage);
    fflush(stdout);
}



/**
 * @brief 编码子设备信息（格式："name,id,version"）
 * @param dest 存储编码后的字符串
 * @param s_name 设备名称
 * @param s_id 设备ID（整数）
 * @param s_version 设备版本
 * @return 成功返回 0，失败返回 -1
 */
int hd_child_info_encode(char *dest, const char *s_name, int s_id, const char *s_version) {
    if (!dest || !s_name || !s_version) {
        return -1; // 参数错误
    }
    // 计算所需缓冲区大小（name + id字符串形式 + version + 2个逗号 + '\0'）
    int id_len = snprintf(NULL, 0, "%d", s_id); // 获取s_id的字符串长度
    int total_len = strlen(s_name) + id_len + strlen(s_version) + 2 + 1;
    
    // 检查目标缓冲区是否足够大（假设调用者已分配足够空间）
    snprintf(dest, total_len, "%s,%d,%s", s_name, s_id, s_version);
    return 0;
}

/**
 * @brief 解码子设备信息（格式："name,id,version"）
 * @param source 源字符串
 * @param s_name 存储设备名称（需提前分配足够空间）
 * @param s_id 存储设备ID（整数，按值返回需改为指针，此处假设按值传递有误，实际应改为 int*）
 * @param s_version 存储设备版本（需提前分配足够空间）
 * @return 成功返回 0，失败返回 -1
 * @note 原函数签名中 s_id 为 int，无法通过参数返回，实际应为 int*！
 */
int hd_child_info_decode(const char *source, char *s_name, int *s_id, char *s_version) {
/*   if (!source || !s_name || !s_id || !s_version) {
        return -1; // 参数错误
    }

    // 复制 source（strtok 会修改原字符串）
    char *input = strdup(source);
    if (!input) {
        return -1; // 内存分配失败
    }

    // 提取 name
    char *token = strtok(input, ",");
    if (!token) {
        free(input);
        return -1;
    }
    strncpy(s_name, token, strlen(token) + 1);

    // 提取 id（字符串转整数）
    token = strtok(NULL, ",");
    if (!token) {
        free(input);
        return -1;
    }
    *s_id = atoi(token); // 或用 strtol 更安全

    // 提取 version
    token = strtok(NULL, ",");
    if (!token) {
        free(input);
        return -1;
    }
    strncpy(s_version, token, strlen(token) + 1);

    free(input);*/

if (!source || !s_name || !s_id || !s_version) return -1;

    // 查找第一个逗号（分隔 name 和 id）
    const char *first_comma = strchr(source, ',');
    if (!first_comma) return -1;

    // 提取 name（从开头到第一个逗号）
    size_t name_len = first_comma - source;
    strncpy(s_name, source, name_len);
    s_name[name_len] = '\0';

    // 查找最后一个逗号（分隔 id 和 version）
    const char *last_comma = strrchr(source, ',');
    if (!last_comma || last_comma == first_comma) return -1;

    // 提取 id（第一个逗号后到最后一个逗号前）
    char id_str[20];
    size_t id_len = last_comma - (first_comma + 1);
    strncpy(id_str, first_comma + 1, id_len);
    id_str[id_len] = '\0';
    *s_id = atoi(id_str);

    // 提取 version（最后一个逗号后到字符串结尾）
    strcpy(s_version, last_comma + 1);
    return 0;
}
