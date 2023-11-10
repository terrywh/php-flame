#ifndef FLAME_CORE_CALLABLE_H
#define FLAME_CORE_CALLABLE_H
#include "value.h"

namespace flame::core {

class callable: public value {
public:
    using value::value;
    callable(const value& val)
    : value(val) {}
    
    value operator ()() const&;
    value operator ()(parameter_list& argv) const&;
    value operator ()(std::initializer_list<value> argv) const&;
};

inline value invoke(std::string_view name) { return callable(name)(); }
inline value invoke(std::string_view name, parameter_list& argv) { return callable(name)(argv); }
inline value invoke(std::string_view name, std::initializer_list<value> argv) { return callable(name)(std::move(argv)); }

void handle_exception();
} // flame::core

#endif // FLAME_CORE_CALLABLE_H