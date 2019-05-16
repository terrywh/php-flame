#pragma once
#include "vendor.h"

namespace flame {

    class controller_master;
    class controller_master_worker {
    public:
        controller_master_worker(controller_master* m, int i);
        void close();
        void close_now();
    private:
        controller_master*            m_;
        int                           i_;
        boost::process::child      proc_;
        boost::process::async_pipe sout_;
        boost::process::async_pipe eout_;
        boost::asio::streambuf     sbuf_;
        boost::asio::streambuf     ebuf_;
        void redirect_output(boost::process::async_pipe& pipe, boost::asio::streambuf& buffer);
    };

    class controller_master {
    public:
        controller_master();
        void initialize(const php::array& options);
        void run();
    private:
        std::vector<controller_master_worker*>   worker_spawn_;
        std::vector<controller_master_worker*>   worker_close_;
        int                                      worker_spawn_count_;
        int                                      worker_count_;
        std::unique_ptr<boost::asio::signal_set> signal_;
        boost::asio::steady_timer tm_inter_;
        boost::asio::steady_timer tm_force_;
        std::thread                        thread_;

        std::string                        ofpath_;
        std::shared_ptr<std::ostream>      offile_;

        void spawn_worker(int i);
        void spawn_worker_next(int i = 0);
        void notify_exit(int i, controller_master_worker* w, int exit_code);
        enum {
            CLOSE_NOTHING,
            CLOSE_WORKER_SIGNALING = 0x0001, // 信号通知 + 超时
            CLOSE_WORKER_TERMINATE = 0x0002, // 立即停止
            CLOSE_WORKER_MASK      = 0x000f,
            CLOSE_FLAG_RESTART     = 0x0010, // 重启
            CLOSE_FLAG_MASTER      = 0x0020, // 停止主进程
            CLOSE_FLAG_MASK        = 0x00f0,
        };
        int close_ = CLOSE_NOTHING;
        void close_worker(int i);
        void close_worker_next(int i = 0);
        void close();

        void reload_worker();
        void reload_output();

        void await_signal();

        const char* system_time();
        friend class controller_master_worker;
    };
}
