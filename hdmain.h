#ifndef HDMAIN_H
#define HDMAIN_H

// 插件状态返回值
typedef enum {
    PLUGIN_CONTINUE,  // 继续执行
    PLUGIN_EXIT       // 退出插件
} plugin_status_t;

// 插件接口函数原型
typedef plugin_status_t (*plugin_entry_t)(void);

// 插件必须实现的入口函数名称
#define PLUGIN_ENTRY_FUNC "plugin_entry"

#endif
