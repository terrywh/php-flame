#pragma once
#include "../vendor.h"
#include "../master_process_manager.h"
#include "../master_logger_manager.h"
#include "../signal_watcher.h"
#include "../master_ipc.h"

namespace flame {
    class master: public std::enable_shared_from_this<master>, public master_process_manager, public signal_watcher, public master_logger_manager, public master_ipc {
    public:
        static inline std::shared_ptr<master> get() {
            return mm_;
        }
        static void declare(php::extension_entry& ext);
        static php::value init(php::parameters& params);
        static php::value run(php::parameters& params);
        static php::value dummy(php::parameters& params);

        master();
        std::ostream& output() override;
    protected:
        std::shared_ptr<master_process_manager> pm_self() override {
            return std::static_pointer_cast<master_process_manager>(shared_from_this());
        }
        virtual std::shared_ptr<signal_watcher> sw_self() override {
            return std::static_pointer_cast<signal_watcher>(shared_from_this());
        }
        virtual std::shared_ptr<master_logger_manager> lm_self() override {
            return std::static_pointer_cast<master_logger_manager>(shared_from_this());
        }
        virtual std::shared_ptr<master_ipc> ipc_self() override {
            return std::static_pointer_cast<master_ipc>(shared_from_this());
        }
        bool on_signal(int sig) override;
        bool on_message(std::shared_ptr<ipc::message_t> msg, socket_ptr sock) override;
    private:
        static std::shared_ptr<master> mm_;
        master_logger* lg_;

        int close_ = 0;
        int stats_ = 0;
        friend class controller;
    };
}