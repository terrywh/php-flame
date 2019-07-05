#pragma once
#include <memory>
#include <streambuf>
#include "ipc.h"

class worker_logger_manager;
class worker_logger_buffer: public std::streambuf {
public:
    worker_logger_buffer(worker_logger_manager* mgr, std::uint8_t idx);
protected:
    int overflow(int ch = EOF) override;
    long xsputn(const char* s, long c) override;
private:
    worker_logger_manager*          mgr_;
    std::shared_ptr<ipc::message_t> msg_;
    std::uint16_t                   cap_;
    std::uint8_t                    idx_;

    void transfer_msg();
};
