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

    access_entry& operator %(const modifier& r) {
        flag_ |= r.flag_;
        return *this;
    }
    std::uint32_t finalize() const {
        return flag_;
    }

  
};

extern access_entry::modifier public_;
extern access_entry::modifier private_;
extern access_entry::modifier protected_;
extern access_entry::modifier static_;
extern access_entry::modifier final_;
extern access_entry::modifier abstract_;

} // flame::core

#endif // FLAME_CORE_ACESS_ENTRY_H
