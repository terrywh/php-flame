#include "../coroutine.h"
#include "process.h"

namespace flame::os {

    void process::declare(php::extension_entry &ext) {
        ext
            .constant({"flame\\os\\SIGTERM", SIGTERM})
            .constant({"flame\\os\\SIGKILL", SIGKILL})
            .constant({"flame\\os\\SIGINT", SIGINT})
            .constant({"flame\\os\\SIGUSR1", SIGUSR1})
            .constant({"flame\\os\\SIGUSR2", SIGUSR2});

        php::class_entry<process> class_process("flame\\os\\process");
        class_process
            .property({"pid", 0})
            .method<&process::__construct>("__construct", {}, php::PRIVATE)
            .method<&process::__destruct>("__destruct")
            .method<&process::kill>("kill", {
                {"signal", php::TYPE::INTEGER, false, true}
            })
            .method<&process::detach>("detach")
            .method<&process::wait>("wait")
            .method<&process::stdout>("stdout")
            .method<&process::stderr>("stderr");
        ext.add(std::move(class_process));
    }

    php::value process::__construct(php::parameters& params) {
        return nullptr;
    }

    php::value process::__destruct(php::parameters& params) {
        if (!detach_) {
            if (!exit_ && !c_->wait_for(std::chrono::milliseconds(10000)))
                c_->terminate();
            if (c_->joinable()) c_->join();
        }
        return nullptr;
    }

    php::value process::kill(php::parameters &params) {
        if (!exit_ && c_->valid()) {
            if (params.size() > 0) ::kill(get("pid"), params[0].to_integer());
            else ::kill(get("pid"), SIGTERM);
        }
        return nullptr;
    }

    php::value process::detach(php::parameters& params) {
        detach_ = true;
        c_->detach();
        return nullptr;
    }

    php::value process::wait(php::parameters &params) {
        if (!exit_ && c_->valid()) {
            ch_.reset(coroutine::current);
            ch_.suspend();
            ch_.reset();
        }
        if (c_->joinable()) c_->join();
        return nullptr;
    }

    php::value process::stdout(php::parameters &params) {
        wait(params);
        if (c_->valid()) return out_.get();
        return nullptr;
    }

    php::value process::stderr(php::parameters &params) {
        wait(params);
        if (c_->valid()) return err_.get();
        return nullptr;
    }
} // namespace flame::os
