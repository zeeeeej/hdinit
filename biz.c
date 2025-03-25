#include <stdio.h>
#include <unistd.h>

void business_logic() {
    static int counter = 0;
    printf("Custom business logic running (%d)...\n", ++counter);
    sleep(5);  // 自定义执行间隔
}

// 用于动态加载的宏定义
#define EXPORT __attribute__((visibility("default")))

EXPORT void business_logic();
