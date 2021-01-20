/* PHP 进程管理 */
#ifndef CORE_CLUSTER_H
#define CORE_CLUSTER_H

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/process.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace core {
    // 进程组
    class cluster {
    public:
        cluster();
        // 判定是否为主进程
        [[nodiscard]] bool is_main() {
            return env_.count("FLAME_CUR_WORKER") == 0;
        }
        unsigned int evaluate_child_process_count();
    private:
        // 当前进程的环境变量
        boost::process::environment env_;
    public:
        class child_process {
            // 工作进程命令行
            std::string cmd_;
            // 子进程
            std::vector<std::unique_ptr<boost::process::child>> child_;
            // 启动指定子进程
            void on_fork(int index);
            // 指定子进程退出
            void on_exit(int index, int exit_code);
        public:
            // 启动
            child_process(unsigned int count, const std::string& cmd);
            // 停止
            ~child_process();
            
            // 强制停止（立即终止所有工作，应仅在主进程调用）
            void stop();
        };
        // 工作线程（子进程内有效）
        class worker_thread {
            std::vector<std::thread> worker_;
            // 防止工作线程退出
            boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
        public:
            worker_thread(unsigned int count);
            
            ~worker_thread();
        };
        
        class handle_signal {
        public:
            handle_signal(child_process* cp, worker_thread* wt);
        private:
            // Zend 引擎内置了信号处理流程，这里不能接管
            // boost::asio::signal_set set_;
        };
    };
}


#endif // CORE_CLUSTER_H