#include "master_logger.h"
#include "master_logger_buffer.h"
#include "util.h"

void master_logger::reload(boost::asio::io_context& io) {
    if(path_.string() != "<clog>") {
        auto fb = new master_logger_buffer(io, path_);
        // fb->open(path_, std::ios_base::app);
        ssb_.reset(fb);
        oss_.reset(new std::ostream(fb));

        fb->persync(); // 启动周期性文件缓冲刷新服务
    }
    if (oss_->fail()) { // 文件打开失败时不会抛出异常，需要额外的状态检查
        std::cerr << "[" << util::system_time() << "] (WARNING) Failed to access logger file, fallback to 'clog'\n";
        oss_.reset(&std::clog, boost::null_deleter());
    }
}

void master_logger::close() {
    oss_.reset(&std::clog, boost::null_deleter());
    // ssb_->close();
    ssb_.reset();
}