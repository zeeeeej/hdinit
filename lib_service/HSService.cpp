#include "HSService.h"
#include <iostream>
#include <ctime>
#include <iomanip>

// 构造函数实现
HSService::HSService(const std::string& name, 
                     const std::string& path, 
                     const std::string& version,
                     Type type)
    : name(name), 
      path(path), 
      version(version), 
      pid(0),
      status(Status::UNREGISTERED),
      last_modified(std::time(nullptr)),
      type(type) {}

// Getter方法实现
std::string HSService::getName() const {
    return name;
}

std::string HSService::getPath() const {
    return path;
}

std::string HSService::getVersion() const {
    return version;
}

int HSService::getPid() const {
    return pid;
}

HSService::Status HSService::getStatus() const {
    return status;
}

std::time_t HSService::getLastModified() const {
    return last_modified;
}

HSService::Type HSService::getType() const {
    return type;
}

// Setter方法实现
void HSService::setName(const std::string& newName) {
    name = newName;
    last_modified = std::time(nullptr);
}

void HSService::setPath(const std::string& newPath) {
    path = newPath;
    last_modified = std::time(nullptr);
}

void HSService::setVersion(const std::string& newVersion) {
    version = newVersion;
    last_modified = std::time(nullptr);
}

void HSService::setPid(int newPid) {
    pid = newPid;
    last_modified = std::time(nullptr);
}

void HSService::setStatus(Status newStatus) {
    status = newStatus;
    last_modified = std::time(nullptr);
}

void HSService::setLastModified(std::time_t newTime) {
    last_modified = newTime;
}

void HSService::setType(Type newType) {
    type = newType;
    last_modified = std::time(nullptr);
}

// 打印方法实现
void HSService::print() const {
    // 状态字符串映射
    const char* statusStrings[] = {
        "未注册", "已停止", "停止中", "已启动", "启动中"
    };

    // 类型字符串映射
    const char* typeStrings[] = {
        "主服务", "次服务", "其他服务"
    };

    // 转换时间为可读格式
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", 
                 std::localtime(&last_modified));

    // 输出格式化信息
    std::cout << "========== 服务信息 ==========\n"
              << "名称: " << name << "\n"
              << "路径: " << path << "\n"
              << "版本: " << version << "\n"
              << "PID: " << pid << "\n"
              << "状态: " << statusStrings[static_cast<int>(status)] << "\n"
              << "最后修改: " << timeBuf << "\n"
              << "类型: " << typeStrings[static_cast<int>(type)] << "\n"
              << "==============================" << std::endl;
}