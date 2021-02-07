#ifndef CORE_LOG_DEFAULT_SINK_H
#define CORE_LOG_DEFAULT_SINK_H

#include "logger.h"
#include "basic_snowflake.h"

namespace core {
    // 默认日志输出目标，系统 ::write 提供文件描述符的原子型写入
    class default_sink: public logger::basic_sink {
    public:
        virtual void prepare(std::ostream& os, logger::severity_t severity) const override;
        virtual void write(const char* data, std::size_t size) const override;
    protected:
        default_sink(int fd);
        int             fd_;
        friend class logger;
    };
    // 标准输出
    class stdout_sink: public default_sink {
    public:
        stdout_sink()
        : default_sink(1) {}
    };
    // 错误输出
    class stderr_sink: public default_sink {
    public:
        stderr_sink()
        : default_sink(2) {}
    };
    // 提供文件数据写入支持
    class file_sink: public default_sink {
    public:
        file_sink(std::string_view path);
        // 文件模式 0764
        static const int file_mode;
    };
}

#endif // CORE_LOG_DEFAULT_SINK_H
