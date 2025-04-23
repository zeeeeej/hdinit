#include <stdio.h>
#include <string.h>
#include <../lib_service/HSService.h>
#include <../lib_service_list/HSServiceList.h>
#include <memory>
#include <iostream>


HSServiceList initializeServices(){
    try
    {
           
        HSServiceList list;
        auto service_main = std::make_shared<HSService>("hsmain", "/root/init/hsmain", "0.0.1", HSService::Type::PRIMARY);
        auto service_log = std::make_shared<HSService>("hslog", "/root/init/hslog", "0.0.1", HSService::Type::SECONDARY);
    
        list.registerService(service_main);
        list.registerService(service_log);
        return list;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return HSServiceList();
    }

}


/**
 * 启动程序
 * -启动服务
 * -启动检测
 * -启动升级
 * 
 * 涉及到
 * ipc通信 心跳 等
 */
int main(int argc, char const *argv[]){
    printf("hello world!\n");

    HSServiceList list = initializeServices();


    list.printAllServices();
    return 0;
}