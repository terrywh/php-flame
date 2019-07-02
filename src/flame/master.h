#pragma once
#include "../vendor.h"
#include "../process_manager.h"
#include "../logger_master.h"
#include "../signal_watcher.h"

namespace flame {
    class master {
    public:
        static inline master* get() {
            return mm_;
        }
        static void declare(php::extension_entry& ext);
        static php::value init(php::parameters& params);
        static php::value run(php::parameters& params);
        static php::value dummy(php::parameters& params);

        master();
        void on_signal(const boost::system::error_code& error, int sig);
    private:
        static master*  mm_;

        process_manager       pm_;
        std::unique_ptr<logger_manager> lm_;
        signal_watcher        sw_;
        int close_ = 0;
        int stats_ = 0;

        friend class controller;
    };
}