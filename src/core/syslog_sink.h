#ifndef FLAME_CORE_LOG_SYSLOG_SINK_H
#define FLAME_CORE_LOG_SYSLOG_SINK_H

#include "logger.h"

namespace core { namespace log {

    class syslog_sink: public logger::basic_sink {
    public:
        syslog_sink(std::string_view name);
        ~syslog_sink();
        virtual void write(logger::severity_t level, std::string_view log) const override;
    private:
        static int option_;
    };
}}

#endif // FLAME_CORE_LOG_SYSLOG_SINK_H
