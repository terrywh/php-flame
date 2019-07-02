#pragma once
#include "vendor.h"
#include <boost/process.hpp>
#include <boost/process/async.hpp>

class logger;
class process_manager;
class process_child {
public:
    process_child(boost::asio::io_context& io, process_manager* m, int i);
    void close(bool force = false);
    void signal(int sig);
private:
    process_manager*        manager_;
    int                       index_;

    boost::process::child      proc_;
    boost::process::async_pipe sout_;
    boost::process::async_pipe eout_;
    std::string                sbuf_;
    std::string                ebuf_;
    void redirect_output(boost::process::async_pipe& pipe, std::string& buffer);

    friend class process_manager;
};