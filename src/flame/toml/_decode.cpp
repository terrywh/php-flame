#include "_decode.h"

namespace flame::toml {
    void decode_inplace(php::string& str) {
        php_stripcslashes(str);
    }
}