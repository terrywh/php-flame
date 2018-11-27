#pragma once
#include "vendor.h"

namespace flame
{
    class coroutine_mutex;
    class mutex: public php::class_base
    {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        php::value lock(php::parameters& params);
        php::value unlock(php::parameters& params);

    private:
        std::shared_ptr<coroutine_mutex> mutex_;
        friend class guard;
    };

    class guard: public php::class_base
    {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        php::value __destruct(php::parameters &params);
    private:
        std::shared_ptr<coroutine_mutex> mutex_;
    };
}
