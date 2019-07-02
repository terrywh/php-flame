#include "logger_worker.h"

std::ostream& logger_worker::stream() {
    // TODO 需要 IPC 支持
    return std::cerr;
}

unsigned int logger_manager_worker::connect(const std::string& filepath) {
    // TODO 需要 IPC 支持
    return 0;
}