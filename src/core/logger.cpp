#include "logger.h"
#include "default_sink.h"
#include "syslog_sink.h"
#include "context.h"
#include "exception.h"

namespace core {

    const std::string logger::severity_str[] = {
        "EMERGENCY",
        "ALERT",
        "CRITICAL",
        "ERROR",
        "WARNING",
        "NOTICE",
        "INFO",
        "DEBUG",
    };
    // 创建默认控制台输出
    logger::logger() {
        sink_.emplace_back(new default_sink(1));
    }
    // 项日志加入一项输出目标
    void logger::add_sink(std::unique_ptr<basic_sink> sink) {
        // 初始状态由框架构建了默认的输出，当用户自行指定时，替换原有输出
        if(sink_.size() == 1 && dynamic_cast<default_sink*>(sink_.front().get())) {
            sink_.clear();
        }
        sink_.emplace_back(std::move(sink));
    }
    
    std::unique_ptr<logger::basic_sink> logger::make_sink(const std::string_view& target) {
        if(target[0] == '.' || target[0] == '/') // 形如 ./xxxx ../xxxx /xxxx 表示文件路径
            return std::unique_ptr<basic_sink>(new file_sink(target));
        else if(target[0] == 's') {
            if(target == "stdout")
                return std::unique_ptr<basic_sink>(new stdout_sink());
            else if(target == "stderr")
                return std::unique_ptr<basic_sink>(new stderr_sink());
            else if(target == "syslog") 
                return std::unique_ptr<basic_sink>(new syslog_sink($context->option.service.name));
        }
        else if(target == "console")
            return std::unique_ptr<basic_sink>(new stdout_sink());
        else if(target == "error")
            return std::unique_ptr<basic_sink>(new stderr_sink());
        
        raise<unknown_error>("failed to create log sink target '{}' unknown", target);
    }
}