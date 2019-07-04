#pragma once
#include "vendor.h"
#include <filesystem>

class coroutine_handler;
class worker_logger_manager;
class worker_logger_buffer;
class worker_logger {
public:
    worker_logger(worker_logger_manager* mgr, const std::filesystem::path& file, std::uint8_t index);
    worker_logger(worker_logger_manager* mgr, const std::filesystem::path& file, std::uint8_t index, bool local);
    std::ostream& stream();
private:
    std::uint8_t           idx_;
    std::filesystem::path path_;
    std::unique_ptr<std::streambuf> wlb_;
    std::unique_ptr<std::ostream>   oss_;
    friend class worker_logger_manager;
};

