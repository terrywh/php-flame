#pragma once
#include <memory>
#include <streambuf>
#include "ipc.h"

class master_logger_manager;
// 实现行数或周期为同步、刷写标志的缓冲区
class master_logger_buffer: public std::filebuf {
public:
    master_logger_buffer(boost::asio::io_context& io, std::filesystem::path path);
    void persync();
    // void close();
protected:
    int  overflow(int ch = EOF) override;
    long xsputn(const char* s, long c) override;
private:
    master_logger_manager*          mgr_;
    boost::asio::steady_timer       tms_;
    unsigned int                    lns_ = 0;
};
