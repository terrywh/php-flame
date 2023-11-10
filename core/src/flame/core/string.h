#ifndef FLAME_CORE_STRING_H
#define FLAME_CORE_STRING_H
#include "value.h"
#include <ostream>
#include <cstdint>

struct _zval_struct;

namespace flame::core {

class string : public value {

public:
    using value::value;
    string(const value& val)
    : value(val) {}
    string(const string& str)
    : value(str) {}
    string(value&& val)
    : value(std::move(val)) {}
    string(string&& str)
    : value(std::move(str)) {}
    ~string();

    const char* data() const;
    char* data();
    std::size_t size() const;

    friend std::ostream& operator <<(std::ostream& os, const string& str) {
        return os.write(str.data(), str.size());
    }
};

} // flame::core

#endif // FLAME_CORE_STRING_H