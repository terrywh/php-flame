#pragma once
#include "vendor.h"
#include "logger.h"

class logger_worker_out: public logger {
public:
    using logger::logger;
    std::ostream& stream() override;
    void write(std::string_view data, bool flush = true) override;
};

class logger_worker_ipc: public logger {
    std::ostream& stream() override;
    void write(std::string_view data, bool flush = true) override;
};

class logger_manager_worker: public logger_manager {
public:
    logger* connect(const std::string& filepath, coroutine_handler& ch) override;
};
