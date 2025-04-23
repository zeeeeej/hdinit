#ifndef HSSERVICE_H
#define HSSERVICE_H

#include <string>
#include <ctime>

class HSService {
public:
    // 状态枚举
    enum class Status {
        UNREGISTERED,   // 未注册
        STOPPED,        // 已停止
        STOPPING,       // 停止中
        STARTED,        // 已启动
        STARTING        // 启动中
    };

    // 类型枚举
    enum class Type {
        PRIMARY,        // 主服务
        SECONDARY,      // 次服务
        OTHER           // 其他服务
    };

    // 构造函数
    HSService(const std::string& name, 
              const std::string& path, 
              const std::string& version,
              Type type = Type::OTHER);

    // Getter方法
    std::string getName() const;
    std::string getPath() const;
    std::string getVersion() const;
    int getPid() const;
    Status getStatus() const;
    std::time_t getLastModified() const;
    Type getType() const;

    // Setter方法
    void setName(const std::string& newName);
    void setPath(const std::string& newPath);
    void setVersion(const std::string& newVersion);
    void setPid(int newPid);
    void setStatus(Status newStatus);
    void setLastModified(std::time_t newTime);
    void setType(Type newType);

    // 打印服务信息
    void print() const;

private:
    std::string name;           // 服务名称
    std::string path;           // 服务程序路径
    std::string version;        // 服务版本
    int pid;                    // 进程ID
    Status status;              // 服务状态
    std::time_t last_modified;  // 最后修改时间
    Type type;                  // 服务类型
};

#endif // HSSERVICE_H