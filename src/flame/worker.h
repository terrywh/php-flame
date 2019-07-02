#pragma once
#include "../vendor.h"
#include "../logger_worker.h"
#include "../signal_watcher.h"

namespace flame {
    class worker {
    public:
        static inline worker* get() {
            return ww_;
        }
        static void declare(php::extension_entry& ext);
        
        // 主流程相关
        static php::value init(php::parameters& params);
        static php::value go(php::parameters& params);
        static php::value on(php::parameters &params);
        static php::value run(php::parameters& params);
        static php::value quit(php::parameters& params);
        // 协程相关
        static php::value co_id(php::parameters& params);
        static php::value co_ct(php::parameters& params);
        // 级联数组设置、读取
        static php::value get(php::parameters& params);
        static php::value set(php::parameters& params);

        worker();
        void on_signal(const boost::system::error_code error, int sig);
    private:
        static worker* ww_;

        std::unique_ptr<logger_manager> lm_;
        signal_watcher sw_;

        int close_ = 0;
        int stats_ = 0;

        friend class controller;
    };
}