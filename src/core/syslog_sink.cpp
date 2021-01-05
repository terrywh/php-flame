#include "syslog_sink.h"
#include <syslog.h>

namespace core { namespace log {
    
    int syslog_sink::option_ = LOG_CONS | LOG_PID;
    
    syslog_sink::syslog_sink(std::string_view name) {
        ::openlog(name.data(), option_, LOG_USER);
    }

    syslog_sink::~syslog_sink() {
        ::closelog();
    }

    void syslog_sink::write(logger::severity_t level, std::string_view log) const {
        ::syslog(static_cast<int>(level), "%s\n", log);
    }
}}