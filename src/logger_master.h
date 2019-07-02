#pragma once
#include "vendor.h"
#include <fstream>
#include "logger.h"

class logger_master: public logger {
private:
    std::shared_ptr<std::ostream> file_;
    std::filesystem::path         path_;
public:
    logger_master(std::filesystem::path path, int index)
    : logger(index)
    , path_(path) {}

    void reload() override;
    std::ostream& stream() override {
        return *file_;
    }
    friend class logger_manager_master;
};

class logger_manager_master: public logger_manager {
public:
    unsigned int connect(const std::string& filepath) override;
private:
    unsigned int index_ = 0;
};