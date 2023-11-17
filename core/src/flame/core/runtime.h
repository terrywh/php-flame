#ifndef FLAME_CORE_RUNTIME_H
#define FLAME_CORE_RUNTIME_H
#include "value.h"
#include <string_view>

namespace flame::core {

class runtime {
    static runtime* ins_;
public:
    static runtime& get() { return *ins_; }
    runtime();
    value G(std::string_view name) const; // GLOBAL
    value C(std::string_view name) const; // CONST
};

} // flame::core

#endif // FLAME_CORE_RUNTIME_H
