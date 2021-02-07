#include "default_sink.h"
#include "context.h"
#include "clock.h"

namespace core {
    // 
    default_sink::default_sink(int fd)
    : fd_(fd) {

    }
    // 
    void default_sink::prepare(std::ostream& os, logger::severity_t level) const {
        auto        tp = clock::get_const_instance().time_point();
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        std::tm*    tm = std::gmtime(&tt);
        // 尽量与 syslog 的形式保持一致
        // priority datetime hostname app pid message_id structured_data message
        fmt::print(os, "<{}> {:%FT%H:%M}T{:%S}Z php-flame/{} {} {} ", 
            logger::severity_str[static_cast<int>(level)],
            *tm, tp.time_since_epoch(),
            $context->status.host,
            $context->option.service.name, $context->status.pid);
    }
    //
    void default_sink::write(const char* data, std::size_t size) const {
        ::write(fd_, data, size);
    }
    // 0764
    const int file_sink::file_mode = S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH;

    file_sink::file_sink(std::string_view path)
    : default_sink(::open(path.data(), O_APPEND | O_CREAT, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH)) {
        
    }
}