
#ifndef __HD_HTTP__
#define __HD_HTTP__

#define HOST "http://101.133.13.114:5000"
#define API_CHECK_UPDATE "/checkupdate"
#define API_DOWNLOAD_FILES "/files"

typedef struct 
{
    char url[1024];
    char version[20];
    char md5[33];
    char filename[50];
} hd_http_check_resp;

int hd_http_init();

/**
 * [更新结果]
 * service_name     :   服务名称  
 * api              :   api 比如service_name为hdmain ,api则为/checkupdate/hdmain 。        
 * 
 */
int hd_http_check_update_url(const char *service_name, char *api);

/**
 * [检查更新]
 * service_name     :   服务名称 
 * resp             :   返回结果         
 * 
 */
int hd_http_check_update(const char *service_name, hd_http_check_resp *resp);

/**
 * [下载文件]
 *  url             :   下载地址
 *  download_path   :   下载的目标path
 *  callback        :   下载进度
 */
int hd_http_download(const char *url, const char *download_path, void (*callback)(double ));



#endif // __HD_HTTP__