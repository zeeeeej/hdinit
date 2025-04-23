#include "HSServiceList.h"
#include <iostream>
#include <algorithm>

HSServiceList::HSServiceList() {
    //initialize();
}

HSServiceList::~HSServiceList() {
    //uninitialize();
}

bool HSServiceList::initialize() {
    servicesByName.clear();
    return true;
}

void HSServiceList::uninitialize() {
    servicesByName.clear();
}

bool HSServiceList::registerService(const std::shared_ptr<HSService>& service) {  
    //std::cout << "尝试注册服务: " << (service ? service->getName() : "nullptr") << std::endl;
    if (!service || service->getName().empty()) {
        return false;
    }

    const std::string& name = service->getName();
    if (servicesByName.find(name) != servicesByName.end()) {
       //std::cout << "注册失败: 服务已存在" << std::endl;
        return false;
    }

    servicesByName[name] = service;
    //std::cout << "注册成功: " << name << std::endl;
    return true;
}

bool HSServiceList::unregisterService(const std::string& serviceName) {
    auto it = servicesByName.find(serviceName);
    if (it == servicesByName.end()) {
        return false;
    }

    servicesByName.erase(it);
    return true;
}

bool HSServiceList::updateServiceStatus(
    const std::string& serviceName,
    HSService::Status newStatus) {
    auto service = findServiceByName(serviceName);
    if (!service) {
        return false;
    }

    bool statusChanged = (newStatus != service->getStatus());

    if (!statusChanged) {
        return true;
    }

    if (statusChanged) {
        service->setStatus(newStatus);
    }

    service->setLastModified(std::time(nullptr));
    return true;
}

std::shared_ptr<HSService> HSServiceList::findServiceByName(const std::string& serviceName) const {
    auto it = servicesByName.find(serviceName);
    return it != servicesByName.end() ? it->second : nullptr;
}

std::shared_ptr<HSService> HSServiceList::findServiceByPid(int pid) const {
    for (const auto& pair : servicesByName) {
        const auto& service = pair.second;
        auto s_pid = service->getPid();
        if (s_pid != 0 && s_pid == pid)
        {
           return service;
        }
        
    }
    return nullptr;
}

std::vector<std::shared_ptr<HSService>> HSServiceList::getAllServices() const {
    std::vector<std::shared_ptr<HSService>> services;
    for (const auto& pair : servicesByName) {
        services.push_back(pair.second);
    }
    return services;
}

bool HSServiceList::isServiceRunning(const std::string& serviceName) const {
    auto service = findServiceByName(serviceName);
    return service && 
           (service->getStatus() == HSService::Status::STARTED || 
            service->getStatus() == HSService::Status::STARTING);
}

void HSServiceList::printAllServices() const {
    std::cout << "========== 服务列表 ==========\n";
    std::cout << "总服务数: " << servicesByName.size() << "\n";
    
    for (const auto& pair : servicesByName) {
        const auto& service = pair.second;
        std::cout << "  " << service->getName() 
                  << " (" << (isServiceRunning(pair.first) ? "运行中" : "已停止") << ")\n";
    }
    std::cout << "============================\n";
}

void HSServiceList::printServiceDetails(const std::string& serviceName) const {
    auto service = findServiceByName(serviceName);
    if (service) {
        service->print();
    } else {
        std::cout << "服务 '" << serviceName << "' 未找到\n";
    }
}

bool HSServiceList::validateServiceName(const std::string& serviceName) const {
    return !serviceName.empty();
}


