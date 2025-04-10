#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"
#include "hd_http.h"
#include "hd_utils.h"
#include <sys/stat.h>
#include <pthread.h>

#define DEBUG 1

#ifdef DEBUG
#include <errno.h>
#endif

// 下载进度回调结构体
struct progress_data {
    void (*callback)(double);
    int dummy;
};

// // 进度数据结构
// struct ProgressData {
//     pthread_mutex_t lock;  // 线程安全锁
//     double last_print;     // 上次打印的进度
// };

// // 线程安全的进度条打印
// void print_progress(double percentage) {
//     const int bar_width = 50;
//     int filled = (int)(percentage * bar_width / 100.0);
    
//     printf("\r[");
//     for (int i = 0; i < bar_width; i++) {
//         if (i < filled) printf("=");
//         else if (i == filled) printf(">");
//         else printf(" ");
//     }
//     printf("] %.1f%%", percentage);
//     fflush(stdout);
// }

// // libcurl 进度回调函数
// static int progress_callback_advance(void *clientp, 
//     double dltotal, 
//     double dlnow, 
//     double ultotal, 
//     double ulnow)
// {
// struct ProgressData *pd = (struct ProgressData *)clientp;
// double percentage = 0;

// if (dltotal > 0) {
// percentage = dlnow * 100.0 / dltotal;
// }

// // 避免频繁刷新（每1%变化才打印）
// pthread_mutex_lock(&pd->lock);
// if (percentage - pd->last_print >= 1.0 || percentage >= 100.0) {
//     //hd_print_progress_double(percentage);
//     pd->last_print = percentage;
// }
// pthread_mutex_unlock(&pd->lock);

// return 0;  // 返回非0会中止传输
// }



int hd_http_init(){
    static int init = 0;
    if (!init)
    {
        init=1;
#ifdef DEBUG
        printf("curl_global_init ... \n");
#endif
        CURLcode code = curl_global_init(CURL_GLOBAL_ALL);  // main() 开始处调用
#ifdef DEBUG
        printf("curl_global_init result %d\n",code);
#endif
    }
    return 0;
}

// 内部使用的JSON解析回调
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char *buffer = (char *)userp;
    strncat(buffer, (char *)contents, realsize);
    return realsize;
}

/**
 * [构建检查更新URL]
 * @param service_name 服务名称
 * @param api 输出的API路径
 * @return 0成功，-1失败
 */
int hd_http_check_update_url(const char *service_name, char *api) {
    if (!service_name || !api) return -1;
    snprintf(api, 1024, "%s%s/%s", HOST, API_CHECK_UPDATE, service_name);
    return 0;
}

/**
 * [检查更新]
 * @param service_name 服务名称
 * @param resp 返回的更新信息结构体
 * @return 0成功，-1失败
 */
int hd_http_check_update(const char *service_name, hd_http_check_resp *resp) {
    CURL *curl;
    CURLcode res;
    char buffer[4096] = {0};
    char api_url[1024];

    if (hd_http_check_update_url(service_name, api_url)) {

        return -1;
    }
#ifdef DEBUG
        printf("hd_http_check_update curl_easy_init %s\n",api_url);
#endif

    curl = curl_easy_init();
    printf("hd_http_check_update curl_easy_init result %s --> %p\n",api_url,curl);
    if (!curl) {
#ifdef DEBUG
        printf("hd_http_check_update curl_easy_init error %d(%s)\n",errno,strerror(errno));
#endif
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
#ifdef DEBUG
        printf("hd_http_check_update curl_easy_perform %d(%s)\n",errno,strerror(errno));
#endif
        curl_easy_cleanup(curl);
        return -1;
    }

    // 使用cJSON解析响应
    cJSON *root = cJSON_Parse(buffer);
    if (!root) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        curl_easy_cleanup(curl);
        return -1;
    }

    // 提取字段
    cJSON *url = cJSON_GetObjectItem(root, "url");
    cJSON *version = cJSON_GetObjectItem(root, "version");
    cJSON *md5 = cJSON_GetObjectItem(root, "md5");

    if (cJSON_IsString(url) && cJSON_IsString(version) && cJSON_IsString(md5)) {
        strncpy(resp->url, url->valuestring, sizeof(resp->url) - 1);
        strncpy(resp->version, version->valuestring, sizeof(resp->version) - 1);
        strncpy(resp->md5, md5->valuestring, sizeof(resp->md5) - 1);

        // resp->url[sizeof(resp->url)-1] = '0';

        // 从URL提取文件名
        char *last_slash = strrchr(resp->url, '/');
        printf("last_slash = %s \n",last_slash);
        if (last_slash) {
            strncpy(resp->filename, last_slash + 1, sizeof(resp->filename) - 1);
        }
    } else {
        cJSON_Delete(root);
        curl_easy_cleanup(curl);
        return -1;
    }

    cJSON_Delete(root);

#ifdef DEBUG
    printf("hd_http_check_update url      = %s \n",resp->url);
    printf("hd_http_check_update version  = %s \n",resp->version);
    printf("hd_http_check_update md5      = %s \n",resp->md5);
    printf("hd_http_check_update filename = %s \n",resp->filename);
#endif   

    curl_easy_cleanup(curl);
    return 0;
}

// 下载进度回调函数
// 正确的进度回调函数
static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    struct progress_data *pd = (struct progress_data *)clientp;
    if (pd->callback && dltotal > 0) {
        int progress = (int)(dlnow / dltotal * 100);
        pd->callback(progress);
// #ifdef DEBUG
//         printf("Progress: %d%%\n", progress);
// #endif
    }
    return 0;
}
// static size_t download_progress(void *ptr, size_t size, size_t nmemb, void *userdata) {
//     struct progress_data *pd = (struct progress_data *)userdata;
//     size_t received = size * nmemb;
    
//     // 这里可以计算实际下载进度（需要获取总大小）
//     // 简化示例：每次回调增加1%
//     if (pd->progress < 100) pd->progress++;
//     if (pd->callback) pd->callback(&pd->progress);
// #ifdef DEBUG
//         printf("hd_http_download fopen %d%%\n",pd->progress);
// #endif
    
//     return received;
// }

/**
 * [下载文件]
 * @param url 下载地址
 * @param download_path 保存路径
 * @param callback 进度回调函数
 * @return 0成功，-1失败
 */
int hd_http_download(const char *url, const char *download_path, void (*callback)(double )) {

#ifdef DEBUG
    printf("hd_http_download url=%s ,download_path=%s  !\n",url,download_path);
#endif

    CURL *curl;
    FILE *fp;
    CURLcode res;
    struct progress_data pd = {callback, 0};

    // struct ProgressData pd;
    // pthread_mutex_init(&pd.lock, NULL);
    // pd.last_print = -1.0;  // 初始值

#ifdef DEBUG
        printf("hd_http_download progress_data %p\n",&pd);
#endif

    curl = curl_easy_init();
    if (!curl) 
    {
#ifdef DEBUG
        printf("hd_http_download curl_easy_init %d(%s)\n",errno,strerror(errno));
#endif
        return -1;
    }

#ifdef DEBUG
        printf("hd_http_download fopen :  %s\n",download_path);
#endif
    // 确保目录存在
    if (hd_if_file_path_not_exists_create_path(download_path)!=0) {
        fprintf(stderr, "Cannot create dir: %s\n", strerror(errno));
        return -1;
    }
    fp = fopen(download_path, "wb");
    if (!fp) {
#ifdef DEBUG
        printf("hd_http_download fopen %d(%s)\n",errno,strerror(errno));
#endif
        curl_easy_cleanup(curl);
        return -1;
    }
#ifdef DEBUG
    printf("hd_http_download curl_easy_setopt ...\n");
#endif
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);  // 禁用自定义数据回调
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);       // 数据直接写入文件
#ifdef DEBUG
    printf("hd_http_download curl_easy_setopt callback\n");
#endif  
    if (callback) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &pd);

        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  // 启用进度回调
        // curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback_advance);
        // curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &pd);
    }
#ifdef DEBUG
    printf("hd_http_download curl_easy_perform\n");
#endif  
    res = curl_easy_perform(curl);
// #ifdef DEBUG
//     printf("hd_http_download curl_easy_perform res: (%s)\n",curl_easy_strerror(res));
// #endif  
    if (res!=0)
    {
        printf("hd_http_download curl_easy_perform %d(%s)\n",errno,strerror(errno));
    }

    fchmod(fileno(fp),0755);

    fclose(fp);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? 0 : -1;
}


