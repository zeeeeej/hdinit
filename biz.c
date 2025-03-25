#include <stdio.h>
#include <unistd.h>
#include "hdmain.h"

// 插件自主控制的业务逻辑
plugin_status_t plugin_entry(void) {
    static int count = 0;
    
    // 示例业务逻辑
    printf("Processing business logic (%d)...\n", ++count);
    sleep(1);
    
    // 插件自行决定是否继续
    if (count >= 10) {
        printf("Business logic completed\n");
        return PLUGIN_EXIT;
    }
    
    return PLUGIN_CONTINUE;
}

// 必须导出的符号
__attribute__((visibility("default")))
plugin_status_t plugin_entry(void);
