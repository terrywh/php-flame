#ifndef PHP_FLAME_CORE_FUNCTION_RUN_WORKER_H
#define PHP_FLAME_CORE_FUNCTION_RUN_WORKER_H

#include <thread>
#include <vector>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>

namespace flame {
namespace core {
    
    class function_run_worker {
    private:
        std::vector<std::thread>     worker_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
    public:
        function_run_worker();
        ~function_run_worker();
        void start();
        void halt();
    };

}
}

#endif // PHP_FLAME_CORE_FUNCTION_RUN_WORKER_H
