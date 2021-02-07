#include "syslog_sink.h"
#include <ostream>
#include <syslog.h>

namespace core {
    
    int syslog_sink::option_ = LOG_CONS | LOG_PID;
    
    syslog_sink::syslog_sink(std::string_view name) {
        ::openlog(name.data(), option_, LOG_USER);
    }

    syslog_sink::~syslog_sink() {
        ::closelog();
    }

    void syslog_sink::prepare(std::ostream& os, logger::severity_t severity) const {
        os.put( static_cast<char>(static_cast<int>(severity)) );
    }
    void syslog_sink::write(const char* data, std::size_t size) const {
        ::syslog(static_cast<int>(data[0]), "%.*s", size-1, data);
    }
}