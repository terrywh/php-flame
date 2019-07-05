#include "master_logger_buffer.h"
#include "master_logger_manager.h"

master_logger_buffer::master_logger_buffer(boost::asio::io_context& io, std::filesystem::path path)
: tms_(io) {
    open(path, std::ios_base::app);
}

int master_logger_buffer::overflow(int ch) {
    char c = ch;
    ch = std::filebuf::overflow(ch);
    
    if(c == '\n' && ++lns_ > 64) { // 积攒的行数超过 64 行刷新
        lns_ = 0;
        pubsync();
        persync();
    }
    return ch;
}

long master_logger_buffer::xsputn(const char* s, long c) {
    c = std::filebuf::xsputn(s, c);

    if(s[c-1] == '\n' && ++lns_ > 64) { // 积攒的行数超过 64 行刷新
        lns_ = 0;
        pubsync();
        persync();
    }
    return c;
}


void master_logger_buffer::persync() {
    tms_.cancel();
    tms_.expires_after(std::chrono::milliseconds(2400));
    tms_.async_wait([this] (const boost::system::error_code& error) {
        if(error) return;
        pubsync();
        persync(); // 每 2400 毫秒对文件进行一次刷新
    });
}

// void master_logger_buffer::close() {
//     tms_.cancel();
// }