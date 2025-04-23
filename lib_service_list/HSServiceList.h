#ifndef HSSERVICLIST_H
#define HSSERVICLIST_H

#include <HSService.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <mutex>

class HSServiceList {
public:
    HSServiceList();
    ~HSServiceList();

    // 初始化/反初始化
    bool initialize();
    void uninitialize();

    // 服务生命周期管理
    bool registerService(const std::shared_ptr<HSService>& service);
    bool unregisterService(const std::string& serviceName);
    
    // 服务查询
    std::shared_ptr<HSService> findServiceByName(const std::string& serviceName) const;
    std::shared_ptr<HSService> findServiceByPid(int pid) const;
    std::vector<std::shared_ptr<HSService> > getAllServices() const;

    // 服务状态检查
    bool isServiceRunning(const std::string& serviceName) const;

    // 信息打印
    void printAllServices() const;
    void printServiceDetails(const std::string& serviceName) const;


    bool updateServiceStatus(const std::string& serviceName,HSService::Status newStatus) ;

private:
    std::unordered_map<std::string, std::shared_ptr<HSService> > servicesByName;
    
    bool validateServiceName(const std::string& serviceName) const;
};

#endif // HSSERVICLIST_H