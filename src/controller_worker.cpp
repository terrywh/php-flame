#include "controller_worker.h"
#include "controller.h"

namespace flame
{
    controller_worker::controller_worker()
    {

    }
    void controller_worker::initialize(const php::array &options)
    {
        
    }
    void controller_worker::run()
    {
        // 工作线程的使用时随机的, 需要保持其一直存在
        auto work = boost::asio::make_work_guard(gcontroller->context_y);
        // 子进程的启动过程:
        // 1. 启动线程池, 并使用线程池运行 context_y
        thread_.resize(3);
        for (int i = 0; i < thread_.size(); ++i)
        {
            thread_[i] = std::thread([this] {
                gcontroller->context_y.run();
            });
        }
        // 2. 启动 context_x 运行
        gcontroller->context_x.run();
        // 3. 确认工作线程停止
        work.reset();
        for (int i = 0; i < thread_.size(); ++i)
        {
            thread_[i].join();
        }
    }
}