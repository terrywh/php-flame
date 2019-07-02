#pragma once
#include "../../vendor.h"
#include "../coroutine.h"
#include <boost/process.hpp>
#include <boost/process/async.hpp>

namespace flame::os {
    class process: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        php::value __destruct(php::parameters& params);
        php::value kill(php::parameters& params);
        php::value wait(php::parameters& params);
        php::value detach(php::parameters& params);
        php::value stdout(php::parameters& params);
        php::value stderr(php::parameters& params);
    private:
        boost::process::child      c_;
        coroutine_handler         ch_;
        std::future<std::string> out_;
        std::future<std::string> err_;
        bool exit_ = false;
        bool detach_ = false;
        friend class php::value spawn(php::parameters& params);
        friend class php::value exec(php::parameters& params);
    };
}
