#pragma once
#include "vendor.h"

class coroutine_handler;
class worker_logger_manager;
class worker_logger_buffer;
class worker_logger {
public:
    worker_logger(worker_logger_manager* mgr, unsigned int index);
    std::ostream& stream() {
        return oss_;
    }
private:
    std::uint8_t idx_;
    std::unique_ptr<worker_logger_buffer> wlb_;
    std::ostream oss_;
    friend class worker_logger_manager;
};

