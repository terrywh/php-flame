#pragma once
#include "vendor.h"

class logger;
class signal_watcher {
public:
    explicit signal_watcher(boost::asio::io_context& io);
    template <class Handler>
    void start(Handler&& cb) {
        ss_->async_wait(cb);
    }
    void close();
    logger*  lg_;
private:
    std::unique_ptr<boost::asio::signal_set> ss_;
    int close_ = 0;
    int stats_ = 0;
};