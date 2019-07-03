#pragma once
#include "vendor.h"
#include <boost/process.hpp>
#include <boost/process/async.hpp>

class master_process_manager;
class master_process {
public:
    master_process(boost::asio::io_context& io, master_process_manager* m, std::uint8_t idx);
    void close(bool force = false);
    void signal(int sig);
private:
    master_process_manager*     mgr_;
    std::uint8_t                idx_;

    boost::process::child      proc_;
    boost::process::async_pipe sout_;
    boost::process::async_pipe eout_;
    std::string                sbuf_;
    std::string                ebuf_;
    void redirect_output(boost::process::async_pipe& pipe, std::string& buffer);

    friend class master_process_manager;
};