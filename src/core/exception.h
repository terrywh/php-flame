#ifndef CORE_EXCEPTION_H
#define CORE_EXCEPTION_H

#include "vendor.h"

namespace core {

    class unknown_error: public std::exception {
    public:
        unknown_error(const std::string& msg)
        : msg_(msg) {}
        const char* what() const noexcept {
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };
    template <class Error, class F, class ...Args>
    [[noreturn]] void raise(F&& format, Args&&... args) {
        throw unknown_error( fmt::format(std::forward<F>(format), std::forward<Args>(args)...) );
    }
}

#endif // CORE_EXCEPTION_H
