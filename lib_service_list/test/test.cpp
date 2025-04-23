#include <iostream>
#include <memory>
#include <thread>
#include "../HSServiceList.h"

using namespace std;

void testBasicOperations() {
    cout << "=== Testing Basic Operations ===" << endl;
    
    HSServiceList serviceList;
    serviceList.initialize();
    
    // 创建几个测试服务
    auto service1 = make_shared<HSService>("Service1", "/path/to/service1", "1.0", HSService::Type::PRIMARY);
    auto service2 = make_shared<HSService>("Service2", "/path/to/service2", "2.0", HSService::Type::SECONDARY);
    auto service3 = make_shared<HSService>("Service3", "/path/to/service3", "3.0");
    
    // 测试注册服务
    cout << "Registering services..." << endl;
    assert(serviceList.registerService(service1));
    assert(serviceList.registerService(service2));
    assert(serviceList.registerService(service3));
    
    // 测试重复注册
    assert(!serviceList.registerService(service1));
    
    // 测试查找服务
    cout << "Testing service lookup..." << endl;
    assert(serviceList.findServiceByName("Service1") != nullptr);
    assert(serviceList.findServiceByName("NonExistent") == nullptr);

    cout << "Testing findServiceByPid..." << endl;
    service1->setPid(1001);
    auto service_find_by_pid = serviceList.findServiceByPid(1001);
    service1->print();
    assert(service_find_by_pid == service1);
    
    // 测试获取所有服务
    auto allServices = serviceList.getAllServices();
    assert(allServices.size() == 3);
    
    // 测试打印功能
    cout << "Printing all services:" << endl;
    serviceList.printAllServices();
    
    cout << "Printing service details:" << endl;
    serviceList.printServiceDetails("Service1");
     
    // 测试服务状态检查
    serviceList.updateServiceStatus("Service1",HSService::Status::STARTED);
    assert(serviceList.isServiceRunning("Service1"));
    
    // 测试注销服务
    cout << "Testing service unregistration..." << endl;
    assert(serviceList.unregisterService("Service1"));
    assert(serviceList.findServiceByName("Service1") == nullptr);
    assert(serviceList.findServiceByPid(1001) == nullptr);
    
    serviceList.uninitialize();
    cout << "Basic operations test passed!" << endl << endl;
}

void testConcurrentAccess() {
    cout << "=== Testing Concurrent Access ===" << endl;
    
    HSServiceList serviceList;
    serviceList.initialize();
    
    auto worker = [&serviceList](int id) {
        string name = "Service" + to_string(id);
        auto service = make_shared<HSService>(name, "/path/to/" + name, "1.0");
        
        // 模拟并发注册
        serviceList.registerService(service);
        
        // 模拟并发查找
        auto found = serviceList.findServiceByName(name);
        assert(found != nullptr);
    };
    
    // 创建多个线程并发操作
    const int numThreads = 10;
    thread threads[numThreads];
    
    for (int i = 0; i < numThreads; ++i) {
        threads[i] = thread(worker, i + 1);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证所有服务都已正确注册
    auto allServices = serviceList.getAllServices();
    assert(allServices.size() == numThreads);
    
    // 验证部分服务有PID映射
    for (int i = 1; i <= numThreads; ++i) {
        string name = "Service" + to_string(i);
        assert(serviceList.findServiceByName(name) != nullptr);
        
        if (i % 2 == 0) {
            assert(serviceList.findServiceByPid(1000 + i) == nullptr);
        }
    }
    
    serviceList.printAllServices();
    serviceList.uninitialize();
    cout << "Concurrent access test passed!" << endl << endl;
}

void testEdgeCases() {
    cout << "=== Testing Edge Cases ===" << endl;
    
    HSServiceList serviceList;
    
    // 测试未初始化
    // try
    // {
    //     auto service = make_shared<HSService>("Test", "/path", "1.0");
    //     assert(!serviceList.registerService(service));
    // }
    // catch(const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }
    
  
    
    // 初始化后测试
    serviceList.initialize();
    
    // 测试空名称
    auto emptyNameService = make_shared<HSService>("", "/path", "1.0");
    assert(!serviceList.registerService(emptyNameService));
    
    // 测试无效服务名查找
    assert(serviceList.findServiceByName("") == nullptr);
    
    // 测试无效PID查找
    assert(serviceList.findServiceByPid(-1) == nullptr);
    
    // 测试注销不存在的服务
    assert(!serviceList.unregisterService("NonExistent"));
    
    serviceList.uninitialize();
    cout << "Edge cases test passed!" << endl << endl;
}

/**
 * export LD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/libs/libhsservice:$LD_LIBRARY
 * export DYLD_LIBRARY_PATH=/Users/xiangpengle/Documents/linux/code/hdinit/libs/libhsservice:$DYLD_LIBRARY_PATH
 * 
 */
int main() {
    testBasicOperations();
    testConcurrentAccess();
    testEdgeCases();
    
    cout << "All tests completed successfully!" << endl;
    return 0;
}