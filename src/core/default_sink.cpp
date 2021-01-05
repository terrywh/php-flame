#include "default_sink.h"
#include "clock.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace core { namespace log {
    // 
    default_sink::default_sink(int fd)
    : fd_(fd) {

    }
    // 
    void default_sink::write(logger::severity_t level, std::string_view msg) const {
        auto        tp = clock::get_const_instance().time_point();
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        std::tm*    tm = std::gmtime(&tt);
        // TODO 优化：是否可以直接将 format 的输出指向该文件描述符以减少此次复制？
        fmt::memory_buffer buffer;

        // priority datetime hostname app pid message_id structured_data message
        fmt::format_to(buffer, "<{}> {:%FT%H:%M}T{:%S}Z ", 
            logger::severity_s[static_cast<int>(level)], *tm, tp.time_since_epoch());
        
        ::write(fd_, buffer.data(), buffer.size());
    }
    // 0764
    const int file_sink::file_mode = S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH;

    file_sink::file_sink(std::string_view path)
    : default_sink(::open(path.data(), O_APPEND | O_CREAT, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH)) {
        
    }
}}