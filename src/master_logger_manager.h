#pragma once
#include "vendor.h"
#include "master_logger.h"

class master_logger_manager {
public:
    master_logger_manager(boost::asio::io_context& io);
    virtual ~master_logger_manager();
    // 日志引用
    master_logger* lm_connect(std::string_view filepath);
    master_logger* lm_get(std::uint8_t idx);
    std::ostream&  lm_get(std::uint8_t idx, bool output);
    // 引用清理
    void lm_destroy(std::uint8_t idx);
    // 日志重载
    void lm_reload();
    void lm_close();
    virtual std::shared_ptr<master_logger_manager> lm_self() = 0;
private:
    boost::asio::io_context&                                   io_;
    std::uint8_t                                            index_ = 0;
    std::map<unsigned int, std::unique_ptr<master_logger>> logger_;
};

inline master_logger* master_logger_manager::lm_get(std::uint8_t idx) {
    auto i = logger_.find(idx);
    return i == logger_.end() ? nullptr : i->second.get();
}

inline std::ostream& master_logger_manager::lm_get(std::uint8_t idx, bool output) {
    auto i = logger_.find(idx);
    return i == logger_.end() ? std::clog : i->second.get()->stream();
}
