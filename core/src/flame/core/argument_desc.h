#ifndef FLAME_CORE_ARGUMENT_DESC_H
#define FLAME_CORE_ARGUMENT_DESC_H
#include "value.h"
#include <memory>
#include <string>
#include <vector>

namespace flame::core {

class argument_desc {
public:
    struct modifier {
        std::uint32_t flag;
    };

    class type {
        std::string            name_; // for class type
        std::uint8_t           code_;
        std::uint32_t          flag_;
    public:
        type(::flame::core::value::type typ)
        : code_(typ.code()), flag_(0) { }
        type(const std::string& name)
        : name_(name), flag_(0) { }
        type(const char* name)
        : name_(name), flag_(0) {}

        void finalize(void* entry) const;
        friend class argument_desc;
    };
    static modifier by_reference;
    static modifier is_variadic;
    static modifier allow_null;
    
private:
    std::string    name_;
    argument_desc::type type_;
    
public:
    argument_desc(const std::string& name, argument_desc::type type, std::uint32_t flag = 0)
    : name_(name)
    , type_(type) {}

   ~argument_desc() = default;

    argument_desc& operator|(argument_desc::modifier m) {
        type_.flag_ |= m.flag;
        return *this;
    }

    void finalize(void* entry) const;
};

argument_desc byval(const std::string& name, argument_desc::type type);
argument_desc byref(const std::string& name, argument_desc::type type);

} // namespace flame::core

#endif // FLAME_CORE_ARGUMENT_DESC_H
