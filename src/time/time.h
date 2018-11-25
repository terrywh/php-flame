#include "../vendor.h"

namespace flame::time
{
    void declare(php::extension_entry &ext);
    std::chrono::time_point<std::chrono::system_clock> now();
    std::string iso();
}