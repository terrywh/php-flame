#include "../vendor.h"
#include <toml/toml.hpp>
namespace toml_parser = llparse::toml;

namespace flame::toml {
    void declare(php::extension_entry &ext);

          void set(php::array& r, std::string_view prefix, std::size_t index, const php::value& v);
    php::value get(php::array  r, std::string_view prefix, std::size_t index);
}
