#ifndef PHP_FLAME_CORE_WORKER_H
#define PHP_FLAME_CORE_WORKER_H

#include <thread>
#include <vector>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>

namespace flame {
namespace core {
    
    class worker {
    private:
        std::vector<std::thread>     worker_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
        static void run_ex();
    public:
        worker();
        void start();
        void stop();
        void halt();
    };

}
}

#endif // PHP_FLAME_CORE_WORKER_H
