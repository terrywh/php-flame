#pragma once
#include "../vendor.h"
#include "../worker_logger_manager.h"
#include "../signal_watcher.h"
#include "../worker_ipc.h"
#include "../coroutine_queue.h"

namespace flame {
    class worker: public std::enable_shared_from_this<worker>, public signal_watcher, public worker_ipc, public worker_logger_manager {
    public:
        static inline std::shared_ptr<worker> get() {
            return worker::ww_;
        }
        static void declare(php::extension_entry& ext);
        // 主流程相关
        static php::value init(php::parameters& params);
        static php::value go(php::parameters& params);
        static php::value on(php::parameters &params);
        static php::value off(php::parameters& params);
        static php::value run(php::parameters& params);
        static php::value quit(php::parameters& params);
        // 协程相关
        static php::value co_id(php::parameters& params);
        static php::value co_ct(php::parameters& params);
        // 级联数组设置、读取
        static php::value get(php::parameters& params);
        static php::value set(php::parameters& params);
        // 传递消息到其他工作进程
        static php::value send(php::parameters& params);
        // 生成 snowflake 兼容唯一标识
        static php::value unique_id(php::parameters& params);
        
        worker(std::uint8_t idx);
        std::ostream& output() override;
    protected:
        virtual std::shared_ptr<signal_watcher> sw_self() override {
            return std::static_pointer_cast<signal_watcher>(shared_from_this());
        }
        virtual std::shared_ptr<worker_ipc> ipc_self() override {
            return std::static_pointer_cast<worker_ipc>(shared_from_this());
        }
        virtual std::shared_ptr<worker_logger_manager> lm_self() override {
            return std::static_pointer_cast<worker_logger_manager>(shared_from_this());
        }
        bool on_signal(int sig) override;
        bool on_message(std::shared_ptr<ipc::message_t> msg) override;
    private:
        static std::shared_ptr<worker> ww_;
        std::list<php::value> msgq_; // notify queue
        coroutine_handler     msgc_; // notify coroutine

        void msg_start();
        void msg_close();
    };
}