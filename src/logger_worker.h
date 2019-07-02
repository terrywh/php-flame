#pragma once
#include "vendor.h"
#include "logger.h"

class logger_worker: public logger {
public:
    std::ostream& stream() override;
};

class logger_manager_worker: public logger_manager {
public:
    unsigned int connect(const std::string& filepath) override;
};
