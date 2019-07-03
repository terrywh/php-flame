#pragma once
#include "vendor.h"
#include <fstream>

class master_logger {
private:
    std::uint8_t                   idx_;
    unsigned int                   ref_;
    std::shared_ptr<std::ostream> file_;
    std::filesystem::path         path_;
public:
    master_logger(std::filesystem::path path, int index): idx_(index), ref_(1), path_(path) {}

    void reload();
    std::ostream& stream() {
        return *file_;
    }
    void close();
    friend class master_logger_manager;
};
