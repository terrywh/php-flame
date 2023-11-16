#ifndef FLAME_CORE_CLASS_CLOSURE_H
#define FLAME_CORE_CLASS_CLOSURE_H
#include "value.h"
#include "parameter.h"
#include <rome/delegate.hpp>
#include <variant>

namespace flame::core {

class parameter_list;
class module_entry;

// 注意：
// 1. 实际模板参数仅允许 4 中参数形态；
class closure {
    std::variant<
        std::nullptr_t,
        rome::delegate<void ()>,
        rome::delegate<void (parameter_list&)>,
        rome::delegate<value ()>,
        rome::delegate<value (parameter_list&)>> fn_;
public:
    closure()
    : fn_(nullptr) {}
    
    value __invoke(parameter_list& argv) {
        if (fn_.index() == 1) {
            std::get<1>(fn_)();
            return {};
        } else if (fn_.index() == 2) {
            std::get<2>(fn_)(argv);
            return {};
        } else if (fn_.index() == 3) {
            return std::get<3>(fn_)();
        } else if (fn_.index() == 4) {
            return std::get<4>(fn_)(argv);
        } else {
            return {};
        }
    }

    template <class S>
    void set_delegate(rome::delegate<S>&& fn) {
        fn_ = std::move(fn);
    }
};

} // flame::core

#endif // FLAME_CORE_CLASS_CLOSURE_H