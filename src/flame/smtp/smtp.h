#include "../../vendor.h"
#include <curl/curl.h>

namespace flame::smtp {
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters& params);
}
