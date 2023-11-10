#ifndef FLAME_CORE_ACESS_ENTRY_H
#define FLAME_CORE_ACESS_ENTRY_H
#include <cstdint>

namespace flame::core {

class access_entry {
public:
    class modifier {
        std::uint32_t flag_;

    public:
        explicit modifier(std::uint32_t flag)
        : flag_ {flag} {}
        access_entry operator |(const modifier& r);
        friend class access_entry;
    };


private:
    std::uint32_t flag_;

public:
    access_entry()
    : flag_{ 0 } {}
    explicit access_entry(std::uint32_t flag)
    : flag_ { flag } {}
    access_entry(const modifier& r)
    : flag_ { r. flag_ } {}

    access_entry& operator |(const modifier& r) {
        flag_ |= r.flag_;
        return *this;
    }

    static access_entry public_;
    static access_entry private_;
    static access_entry protected_;
};


} // flame::core

#endif // FLAME_CORE_ACESS_ENTRY_H
