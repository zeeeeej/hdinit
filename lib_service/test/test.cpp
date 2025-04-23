#include "../HSService.h"

int main(int argc,char ** argv) {
    // 创建一个主服务实例
    HSService service("数据库服务", "/usr/bin/dbservice", "1.2.3", HSService::Type::PRIMARY);
    
    // 设置服务状态
    service.setPid(12345); // 设置进程ID
    service.setStatus(HSService::Status::STARTED); // 设置为已启动状态
    
    // 打印服务信息
    service.print();
    
    return 0;
}