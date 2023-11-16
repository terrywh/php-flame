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
// xxxxx
// yyyyy
core::value hello3() {
    core::callable rand {"rand"};
    core::invoke("srand", {123456});
    return rand();
}

core::value hello4(core::parameter_list& argv) {
    core::object date = argv[0];
    core::string str = date.call("format", {"Y-m-d H:i:s O T"});
    core::object obj = argv[1];
    std::cout << "hello (#4) [" << static_cast<std::string_view>(obj.get("name")) << "]! \n";
    return str;
}

class hello5 : public core::class_basic<hello5> {
public:
    hello5() {
        std::cout << "create\n";
    }

    void hello() {
        std::string_view name = zobj()->get("name");
        std::cout << "hello (#5) [" << name << "]!\n";
    }

    ~hello5() {
        std::cout << "destroy\n";
    }
};

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
            + core::ini("demo.hello", "hello (#6) (world)!") % core::ini_entry::all
            + core::constant("demo\\which", 0)
            + core::function<hello1>("hello1")
            + core::function<hello2>("hello2", {
                core::byval("name", core::value::type::string),
                core::byval("size", core::value::type::integer)
            })
            + core::function<hello3>("hello3", core::value::type::string)
            + core::function<hello4>("hello4", core::value::type::string, {
                core::byval("time", "DateTime"),
                core::byval("name", core::value::type::object)
            });
        
        demo.declare<hello5>("hello5")
            + core::method<hello5, &hello5::hello>("hello")
            + core::property("index", 5) % core::static_
            + core::constant("which", 6)
            + core::property("name", "default");
        
        return demo;
    }
}