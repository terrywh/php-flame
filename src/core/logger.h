#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <boost/serialization/singleton.hpp>
#include <memory>
#include <vector>
#include <string_view>
#include <syslog.h> // for constants

namespace core { namespace log {
    // 日志
    class logger: public boost::serialization::singleton<logger> {
    public:
        // 信息严重程度 (0~7)
        enum class severity_t {
            EMERGENCY = LOG_EMERG, // 0
            ALERT = LOG_ALERT,     // 1
            CRITICAL = LOG_CRIT,   // 2
            ERROR = LOG_ERR,       // 3
            WARNING = LOG_WARNING, // 4
            NOTICE = LOG_NOTICE,   // 5
            INFO = LOG_INFO,       // 6
            DEBUG = LOG_DEBUG,     // 7
        };
        // 信息严重程度对应文本
        static const char* severity_s[];
        // 日志的存储目标
        class basic_sink {
        public:
            virtual void write(severity_t level, std::string_view log) const = 0;
        };
        // 构建默认日志器
        logger();
        // 项日志加入一项输出目标
        void add_sink(std::unique_ptr<basic_sink> sink);
        //
        static std::unique_ptr<basic_sink> make_sink(const std::string_view& target);
    private:
        // 日志的输出目标
        std::vector<std::unique_ptr<basic_sink>> sink_;
    };
}}

#endif // CORE_LOGGER_H
