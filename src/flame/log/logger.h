#pragma once
#include "../../vendor.h"

class logger;
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
        };
        static std::array<std::string, 6> LEVEL_STR;
        static int LEVEL_OPT;
        ~logger();
        void write_ex(int lv, php::parameters& params);
        php::value trace(php::parameters &params);
        php::value debug(php::parameters &params);
        php::value info(php::parameters &params);
        php::value warning(php::parameters &params);
        php::value error(php::parameters &params);
        php::value fatal(php::parameters &params);

        void set_logger(::logger* lg) {
            lg_ = lg;
        }
    private:
        ::logger* lg_ = nullptr;

        friend php::value connect(php::parameters& params);
    };
}