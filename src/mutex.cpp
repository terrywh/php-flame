#include "coroutine.h"
#include "coroutine_mutex.h"
#include "mutex.h"

namespace flame {
    void mutex::declare(php::extension_entry &ext) {
        php::class_entry<mutex> class_mutex("flame\\mutex");
        class_mutex
            .method<&mutex::__construct>("__construct", {
                {"n", php::TYPE::INTEGER, false, true},
            })
            .method<&mutex::lock>("lock")
            .method<&mutex::unlock>("unlock");
        ext.add(std::move(class_mutex));
    }

    php::value mutex::__construct(php::parameters& params) {
        mutex_.reset(new coroutine_mutex());
        return nullptr;
    }

    php::value mutex::lock(php::parameters& params) {
        coroutine_handler ch {coroutine::current};
        mutex_->lock(ch);
        return nullptr;
    }

    php::value mutex::unlock(php::parameters& params) {
        mutex_->unlock();
        return nullptr;
    }

    void guard::declare(php::extension_entry &ext) {
        php::class_entry<guard> class_guard("flame\\guard");
        class_guard
            .method<&guard::__construct>("__construct", {
                {"mutex", "flame\\mutex"},
            })
            .method<&guard::__destruct>("__destruct");
        ext.add(std::move(class_guard));
    }

    php::value guard::__construct(php::parameters & params) {
        php::object obj = params[0];
        mutex_ = static_cast<mutex*>(php::native(obj))->mutex_;
        coroutine_handler ch {coroutine::current};
        mutex_->lock(ch);
        return nullptr;
    }

    php::value guard::__destruct(php::parameters & params) {
        mutex_->unlock();
        return nullptr;
    }
}
