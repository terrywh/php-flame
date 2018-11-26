#pragma once
#include "vendor.h"
#include "coroutine_queue.h"

namespace flame
{
    class queue: public php::class_base 
    {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters& params);
        php::value push(php::parameters &params);
        php::value pop(php::parameters &params);
        php::value close(php::parameters &params);
    private:
        std::shared_ptr<coroutine_queue<php::value>> q_;

        friend php::value select(php::parameters &params);
    };
}