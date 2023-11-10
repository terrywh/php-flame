#ifndef FLAME_CORE_ERROR_H
#define FLAME_CORE_ERROR_H

#include "value.h"
#include <stdexcept>
#include <string>

namespace flame::core {

class exception : public std::exception {
    std::string what_;
public:
    exception(const std::string& what)
    : what_(what) {}
    const char* what() const noexcept {
        return what_.data();
    }
};

class type_error : public exception {
public:
    type_error(value::type expect, const value& actual);
    type_error(value::type expect);
};

void throw_exception(const exception& e);

} // flame::core

#endif // FLAME_CORE_ERROR_H
