#include "hd_logger.h"
#include <stdarg.h>
#include <stdlib.h>

// 当前日志级别，默认为INFO
static HDLoggerLevel current_logger_level = HD_LOGGER_LEVEL_INFO;
static HDLoggerDateTimeFormat current_logger_datetime_format = HD_LOGGER_DATETIME_ONLY_TIME;

// 设置日志级别
void hd_logger_set_level(HDLoggerLevel level) {
    current_logger_level = level;
}

void hd_logger_set_dateformat(HDLoggerDateTimeFormat format){
    current_logger_datetime_format = format;
}

// 获取当前时间字符串
static void get_time_string(char* buffer, size_t buffer_size) {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    if (current_logger_datetime_format == HD_LOGGER_DATETIME_ONLY_TIME){
        strftime(buffer, buffer_size, "%H:%M:%S", timeinfo);
    } else{
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", timeinfo);
    }
}

// 获取日志级别字符串
static const char* get_level_string(HDLoggerLevel level) {
    switch(level) {
        case HD_LOGGER_LEVEL_DEBUG:   return "D";
        case HD_LOGGER_LEVEL_INFO:    return "I";
        case HD_LOGGER_LEVEL_WARNING: return "W";
        case HD_LOGGER_LEVEL_ERROR:   return "E";
        case HD_LOGGER_LEVEL_FATAL:   return "F";
        default:                return "UNKNOWN";
    }
}

// 日志打印函数
void hd_logger_print(HDLoggerLevel level, const char* tag, const char* msg, ...) {
    // 如果日志级别低于设置级别，则不打印
    if (level < current_logger_level) {
        return;
    }
    
    char time_buffer[20];
    get_time_string(time_buffer, sizeof(time_buffer));
    
    // 打印日志头
    fprintf(stderr, "%s [%s] [%-8s] ", time_buffer, get_level_string(level), tag);
    
    // 打印日志内容
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    
    // 换行
    // fprintf(stderr, "\n");
    
    // 如果是FATAL级别，退出程序
    //if (level == HD_LOGGER_LEVEL_FATAL) {
    //    exit(EXIT_FAILURE);
    //}
}