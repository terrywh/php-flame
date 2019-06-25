#pragma once
#include "../vendor.h"
#include "toml.h"

namespace flame::toml {

class _executor {
public:
    _executor(php::array& root) noexcept
    : root_(root) {}
    void operator ()(const toml_parser::parser& p, std::string_view chunk);
    void operator ()(const toml_parser::parser& p);

private:
    php::array&   root_;
    php::buffer buffer_;

    static php::value restore(php::string& raw, std::uint8_t value_type);
};


}