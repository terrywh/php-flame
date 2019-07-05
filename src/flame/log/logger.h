#pragma once
#include "../../vendor.h"

class coroutine_handler;
class worker_logger;
namespace flame::log {

    class logger: public php::class_base {
    public:
        enum {
            LEVEL_TRACE,
            LEVEL_DEBUG,
            LEVEL_INFO,
            LEVEL_WARNING,
            LEVEL_ERROR,
            LEVEL_FATAL,

            LEVEL_EMPTY,
        };
        static void declare(php::extension_entry& ext);
        static std::array<std::string, 6> LEVEL_STR;
        static int LEVEL_OPT;
        php::value __construct(php::parameters& params);
        // 使用父类型 coroutine_handler 引用能够兼容 C++ / PHP 协程
        void connect(const std::string& path, ::coroutine_handler& ch);
        void write_ex(int lv, php::parameters& params);
        std::ostream& stream();
        php::value write(php::parameters &params);
        php::value trace(php::parameters &params);
        php::value debug(php::parameters &params);
        php::value info(php::parameters &params);
        php::value warning(php::parameters &params);
        php::value error(php::parameters &params);
        php::value fatal(php::parameters &params);
    private:
        std::shared_ptr<worker_logger> lg_;
    };
    // 默认日志记录器
    extern logger* logger_;
}