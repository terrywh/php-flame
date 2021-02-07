#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include "vendor.h"

namespace core {
    // 日志
    class logger {
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
        class sbuffer: public std::basic_stringbuf<char> {
        public:
            const char* data() {
                return pbase();
            }
            std::size_t size() { 
                // 参考 stringbuf::str() 实现
                return pptr() > egptr()
                    ? pptr() - pbase()
                    : pptr() - egptr();
            }
        };
        // 信息严重程度对应文本
        static const std::string severity_str[8];
        // 日志的存储目标
        class basic_sink {
        public:
            virtual void prepare(std::ostream& os, severity_t severity) const = 0;
            virtual void write(const char* data, std::size_t size) const = 0;
        };
        // 构建默认日志器
        logger();
        // 项日志加入一项输出目标
        void add_sink(std::unique_ptr<basic_sink> sink);
        //
        void add_sink(const std::string_view& target) {
            add_sink(make_sink(target));
        }
        // 
        template <class Callback>
        void write(severity_t severity, Callback&& writer) const {
            for(auto& sink: sink_) {
                sbuffer sb;
                std::ostream os{&sb};
                sink->prepare(os, severity);
                writer(os);
                os.put('\n');
                sink->write(sb.data(), sb.size());
            }
        }
        //
        static std::unique_ptr<basic_sink> make_sink(const std::string_view& target);
    private:
        // 日志的输出目标
        std::vector<std::unique_ptr<basic_sink>> sink_;
    };
}

#endif // CORE_LOGGER_H
