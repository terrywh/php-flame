#ifndef FLAME_CORE_INI_ENTRY_H
#define FLAME_CORE_INI_ENTRY_H
#include <string>
struct _zend_ini_entry_def;

namespace flame::core {

class ini_entry {
    std::string key_, val_;
    int mod_;
public:
    enum modifiable { // 可修改范围
        usr = (1<<0),
        dir = (1<<1),
        sys = (1<<2),
        all = usr | dir | sys,
    };

    ini_entry(const std::string& key, const std::string& val)
    : key_(key), val_(val), mod_(0) {}

    ini_entry&& operator %(modifiable m) && {
        mod_ |= m;
        return std::move(*this);
    }

    void finalize(void* entry);
};

ini_entry ini(const std::string& key, const std::string& val);

} // flame::core

#endif // FLAME_CORE_INI_ENTRY_H
