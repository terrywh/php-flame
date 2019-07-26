#include "controller.h"
#include "worker.h"
// #include "../logger_master.h"
// #include "../logger_worker.h"

namespace flame {
    // 全局控制器
    std::unique_ptr<controller> gcontroller;

    controller::controller()
    : type(process_type::UNKNOWN)
    , env(boost::this_process::environment())
    , status(STATUS_UNKNOWN)
    , evnt_cb(new std::multimap<std::string, php::callable>()) {

        worker_size = std::atoi(env["FLAME_MAX_WORKERS"].to_string().c_str());
        worker_size = std::min(std::max((int)worker_size, 0), 256);
        // FLAME_MAX_WORKERS 环境变量会被继承, 故此处顺序须先检测子进程
        if (env.count("FLAME_CUR_WORKER") > 0) {
            type = process_type::WORKER;
            worker_idx = std::atoi(env["FLAME_CUR_WORKER"].to_string().c_str());
        }
        else if (worker_size > 0) type = process_type::MASTER;
        else { // 单进程模式
            worker_size = 0;
            type = process_type::WORKER;
        }
        mthread_id = std::this_thread::get_id();
    }

    controller *controller::on_init(std::function<void (const php::array &options)> fn) {
        init_cb.push_back(fn);
        return this;
    }

    void controller::init(php::array options) {
        for (auto fn : init_cb) fn(options);
    }

    controller* controller::on_stop(std::function<void ()> fn) {
        stop_cb.push_back(fn);
        return this;
    }

    void controller::stop() {
        for (auto fn : stop_cb) fn();
        evnt_cb.reset();
    }

    controller* controller::add_event(const std::string& event, php::callable cb) {
        evnt_cb->insert({event, cb});
        return this;
    }

    controller* controller::del_event(const std::string& event) {
        evnt_cb->erase(event);
        return this;
    }

    std::size_t controller::cnt_event(const std::string& event) {
        return evnt_cb->count(event);
    }

    void controller::event(const std::string& event, std::vector<php::value> params) {
        auto ft = evnt_cb->equal_range(event);
        for(auto i=ft.first; i!=ft.second; ++i) i->second.call(params);
    }

    void controller::event(const std::string& event) {
        auto ft = evnt_cb->equal_range(event);
        for(auto i=ft.first; i!=ft.second; ++i) i->second.call();
    }
}
