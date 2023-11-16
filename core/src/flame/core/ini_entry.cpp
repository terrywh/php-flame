#include "ini_entry.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_ini.h>
#include <vector>

namespace flame::core {

void ini_entry::finalize(void* entry) {
    auto* e = reinterpret_cast<zend_ini_entry_def*>(entry);
    e->name = key_.data();
    e->name_length = key_.size();
    e->value = val_.data();
    e->value_length = val_.size();
    if (mod_ == 0) mod_ = all; // all
    e->modifiable = mod_;
}

ini_entry ini(const std::string& key, const std::string& val) {
    return {key, val};
}

} // flame::core