#pragma once
#include "vendor.h"

class logger;
class signal_watcher {
public:
    explicit signal_watcher(boost::asio::io_context& io)
    : ss_(new boost::asio::signal_set(io)) {
        ss_->add(SIGINT);
        ss_->add(SIGTERM);
        ss_->add(SIGUSR1);
        ss_->add(SIGUSR2);
        ss_->add(SIGQUIT);
    }
    virtual ~signal_watcher() {
        ss_.reset();
        // std::cout << "~signal_watcher\n";
    }
    void sw_watch() {
        ss_->async_wait([this, self = sw_self()] (const boost::system::error_code& error, int sig) {
            if (error) return;
            if (!on_signal(sig)) {
                sw_close();
                return;
            }
            sw_watch();
        });
    }
    void sw_close() {
        ss_.reset();
    }
    virtual std::shared_ptr<signal_watcher> sw_self() = 0;
protected:
    virtual bool on_signal(int sig) = 0;
private:
    std::unique_ptr<boost::asio::signal_set> ss_;
};