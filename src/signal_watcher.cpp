#include "signal_watcher.h"
#include "logger.h"

signal_watcher::signal_watcher(boost::asio::io_context& io)
: ss_(new boost::asio::signal_set(io)) {
    ss_->add(SIGINT);
    ss_->add(SIGTERM);
    ss_->add(SIGUSR1);
    ss_->add(SIGUSR2);
}

void signal_watcher::close() {
    ss_.reset();
}