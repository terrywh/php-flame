#ifndef FLAME_CORE_LOG_SYSLOG_SINK_H
#define FLAME_CORE_LOG_SYSLOG_SINK_H

#include "logger.h"

namespace core {

    class syslog_sink: public logger::basic_sink {
    public:
        syslog_sink(std::string_view name);
        ~syslog_sink();
        virtual void prepare(std::ostream& os, logger::severity_t severity) const override;
        virtual void write(const char* data, std::size_t size) const override;
    private:
        static int option_;
    };
}

#endif // FLAME_CORE_LOG_SYSLOG_SINK_H
