#ifndef FLAME_CORE_ARGUMENT_ENTRY_H
#define FLAME_CORE_ARGUMENT_ENTRY_H
#include "value.h"
#include <string>
#include <vector>

namespace flame::core {

class argument {
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
        friend class argument_entry;
    };
    static modifier by_reference;
    static modifier is_variadic;
    static modifier allow_null;
};

class argument_entry {
private:
    std::string    name_;
    argument::type type_;
    
public:
    argument_entry(const std::string& name, argument::type type, std::uint32_t flag = 0)
    : name_(name)
    , type_(type) {}

   ~argument_entry() = default;

    argument_entry& operator|(argument::modifier m) {
        type_.flag_ |= m.flag;
        return *this;
    }

    void finalize(void* entry) const;
};

argument_entry byval(const std::string& name, argument::type type);
argument_entry byref(const std::string& name, argument::type type);

class argument_entry_list {
    argument::type        type_; // return type
    std::vector<argument_entry> list_;

public:
    argument_entry_list(argument::type type, std::initializer_list<argument_entry> list);
    argument_entry_list(argument_entry_list&& m) = default;
   ~argument_entry_list() = default;
    
    std::size_t size() const;
    void * finalize();
};


} // namespace flame::core

#endif // FLAME_CORE_ARGUMENT_ENTRY_H
