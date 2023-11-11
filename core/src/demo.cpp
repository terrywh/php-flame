#include "flame/core/entry.h"
#include "flame/core.h"
#include <iostream>
namespace core = flame::core;

void hello1() {
    std::cout << "hello (#1) (world)!\n"; 
}

void hello2(core::parameter_list& argv) {
    core::string name = argv[0];
    std::int64_t size = argv[1];
    std::cout << "hello (#" << size << ") [" << name << "]!\n"; 
}

core::value hello3() {
    core::callable rand {"rand"};
    core::invoke("srand", {123456});
    return rand();
}

core::value hello4(core::parameter_list& argv) {
    core::object obj = argv[0];
    return obj.call("format", {"Y-m-d H:i:s O T"});
}

class hello5 {};

extern "C" {
    FLAME_PHP_EXPORT void *get_module() {
        static core::module_entry demo {"demo", "0.20.0"};
        demo
            + core::on_module_start([] () {
                std::cout << "module started!\n";
            })
            + core::on_module_stop([] () {
                std::cout << "module stopped!\n";
            })
            + core::function<hello1>("hello1")
            + core::function<hello2>("hello2", {
                core::byval("name", core::value::type::string),
                core::byval("size", core::value::type::integer)
            })
            + core::function<hello3>("hello3", core::value::type::string)
            + core::function<hello4>("hello4", core::value::type::string, {
                core::byval("time", "DateTime")
            });
        core::class_entry<hello5> hello5_("hello5");
        demo + hello5_;

        return demo;
    }
}