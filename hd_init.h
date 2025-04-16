#ifndef __HD_INIT__
#define __HD_INIT__

#include "hd_service.h"
#include "hd_context.h"

#define VERSION "1.0.0.1"

#define SPIT ""
#define TAG "hdinit"
#define DEBUG 0
#define MONITOR_SERVICES_INTERVAL 30*60
#define UPGRADE_SERVICES_INTERVAL 24*60*60
#define MAX_SERVICES 20
#define MAX_RESTART_MAIN 5
#define ERROR_CHILD_START_FAIL 99

#if HD_CONTEXT
#define HD_INIT_ROOT "./.hdinit"
#else
#define HD_INIT_ROOT "/root/.hdinit"
#endif

#define HD_INIT_SERVICE_MAIN "hdmain"
#if HD_CONTEXT
#define HD_INIT_SERVICE_MAIN_PATH "./.service/hdmain"
#else
#define HD_INIT_SERVICE_MAIN_PATH "/root/hdmain"
#endif

#define HD_INIT_SERVICE_LOG "hdlog"
#if HD_CONTEXT
#define HD_INIT_SERVICE_LOG_PATH "./.service/hdlog"
#else
#define HD_INIT_SERVICE_LOG_PATH "/root/hdlog"
#endif

#define HD_INIT_SERVICE_SHELL "hdshell"

#if HD_CONTEXT
#define HD_INIT_SERVICE_SHELL_PATH "./.service/hdshell"
#else
#define HD_INIT_SERVICE_SHELL_PATH "/root/hdshell"
#endif

#if DEBUG
#define DEBUG_THREAD
#else
#endif


/**
 * 所有服务
 */
HDServiceArray g_service_array;

/**
 * 服务管理是否运行
 */
atomic_int service_manager_running = 1;

/**
 * 连续启动主服务次数
 */
atomic_int start_main_service_count = 0;

atomic_int service_manager_running_count = 0;

atomic_int ping_pong_index = 0;

#endif // __HD_INIT__
