#pragma once
#include "vendor.h"
#include <fstream>

class master_logger {
private:
    std::uint8_t                     idx_;
    unsigned int                     ref_;
    std::shared_ptr<std::ostream>    oss_;
    std::unique_ptr<std::streambuf>  ssb_;
    std::filesystem::path           path_;
public:
    master_logger(std::filesystem::path path, int index): idx_(index), ref_(1), path_(path) {}
    std::uint8_t index() {
        return idx_;
    }
    void reload(boost::asio::io_context& io);
    std::ostream& stream() {
        return *oss_;
    }
    void close();
    friend class master_logger_manager;
};
