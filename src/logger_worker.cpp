#include "logger_worker.h"
#include "coroutine.h"

std::ostream& logger_worker_out::stream() {
    return std::clog;
}

void logger_worker_out::write(std::string_view data, bool flush) {
    std::clog << data;
    if(flush) std::clog.flush();
}

logger* logger_manager_worker::connect(const std::string& filepath, coroutine_handler& ch) {
    // TODO 实现基于 IPC 的通道支持

    auto i = logger_.find(0);
    if(i == logger_.end()) {
        std::cout << "logger_manager_worker::connect\n";
        logger_[0].reset(new logger_worker_out(0));
    }
    return logger_[0].get();
}