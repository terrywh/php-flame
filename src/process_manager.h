#pragma once
#include "vendor.h"
#include "coroutine.h"
#include "process_child.h"

class logger;
class process_manager {
public:
    process_manager(boost::asio::io_context& io, unsigned int count);
    void start();
    void restart(coroutine_handler& ch);
    void close(unsigned int timeout_ms, coroutine_handler& ch);
    void signal(int signal);
    unsigned int count() {
        return count_;
    }
    logger*        lg_ = nullptr; // 等待填充
private:
    boost::asio::io_context& io_;
    unsigned int          count_;

    std::vector<std::unique_ptr<process_child>> child_;

    void on_child_start(process_child* w);
    void on_child_close(process_child* w, bool normal);

    coroutine_handler ch_close_;
    coroutine_handler ch_start_;
    
    friend class process_child;
};
