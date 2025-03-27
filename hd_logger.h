#ifndef __HD_LOGGER__
#define __HD_LOGGER__

#include <stdio.h>
#include <time.h>
#include <string.h>

// 日志级别定义
typedef enum {
    HD_LOGGER_LEVEL_DEBUG,
    HD_LOGGER_LEVEL_INFO,
    HD_LOGGER_LEVEL_WARNING,
    HD_LOGGER_LEVEL_ERROR,
    HD_LOGGER_LEVEL_FATAL
} HDLoggerLevel;

typedef enum {
    HD_LOGGER_DATETIME_FORMAT_ALL,
    HD_LOGGER_DATETIME_ONLY_TIME
} HDLoggerDateTimeFormat;

// 设置日志级别
void hd_logger_set_level(HDLoggerLevel level);

// 设置日志级别
void hd_logger_set_dateformat(HDLoggerDateTimeFormat format);

// 日志打印函数
void hd_logger_print(HDLoggerLevel level, const char* tag, const char* msg, ...);

// 简化宏定义
#define HD_LOGGER_DEBUG(tag, msg, ...)   hd_logger_print(HD_LOGGER_LEVEL_DEBUG, tag, msg, ##__VA_ARGS__)
#define HD_LOGGER_INFO(tag, msg, ...)    hd_logger_print(HD_LOGGER_LEVEL_INFO, tag, msg, ##__VA_ARGS__)
#define HD_LOGGER_WARNING(tag, msg, ...) hd_logger_print(HD_LOGGER_LEVEL_WARNING, tag, msg, ##__VA_ARGS__)
#define HD_LOGGER_ERROR(tag, msg, ...)   hd_logger_print(HD_LOGGER_LEVEL_ERROR, tag, msg, ##__VA_ARGS__)
#define HD_LOGGER_FATAL(tag, msg, ...)   hd_logger_print(HD_LOGGER_LEVEL_FATAL, tag, msg, ##__VA_ARGS__)

#endif // __HD_LOGGER__