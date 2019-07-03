#pragma once
#include "ipc.h"

class worker_ipc;
class worker_logger;
class worker_logger_manager {
public:
    worker_logger_manager(worker_ipc* c): ipc_(c) {}
    virtual ~worker_logger_manager();
    // 连接到指定的日志文件
    std::shared_ptr<worker_logger> lm_connect(const std::string& filepath, coroutine_handler& ch);
    void                           lm_destroy(std::uint8_t idx);
    void lm_close();
protected:
    virtual std::shared_ptr<worker_logger_manager> lm_self() = 0;
private:
    std::map<std::uint8_t, std::weak_ptr<worker_logger>> logger_;
    worker_ipc* ipc_; // 目前的继承实现方式，此字段与 this 相等
    friend class worker_logger;
    friend class worker_logger_buffer;
};
