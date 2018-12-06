#include "controller_worker.h"
#include "controller.h"

namespace flame
{
    controller_worker::controller_worker()
    : signal_(new boost::asio::signal_set(gcontroller->context_y, SIGINT, SIGTERM))
    {

    }
    void controller_worker::initialize(const php::array &options)
    {
        
    }
    void controller_worker::run()
    {
        // 1. 停止信号调用退出通知
        signal_->async_wait([this] (const boost::system::error_code &error, int sig)
        {
            if(error) return;
            signal_.reset();
            boost::asio::post(gcontroller->context_x, [] () {
                gcontroller->status |= controller::controller_status::STATUS_SHUTDOWN;
                auto ft = gcontroller->cbmap->equal_range("quit");
                for (auto i = ft.first; i != ft.second; ++i)
                {
                    // TODO 修复: 需要在协程调用退出回调
                    i->second.call();
                }
            });
        });
        // 工作线程的使用时随机的, 需要保持其一直存在
        auto work = boost::asio::make_work_guard(gcontroller->context_y);
        // 子进程的启动过程:
        // 2. 启动线程池, 并使用线程池运行 context_y
        thread_.resize(8);
        for (int i = 0; i < thread_.size(); ++i)
        {
            thread_[i] = new std::thread([this] {
                gcontroller->context_y.run();
            });
        }
        // 3. 启动 context_x 运行
        gcontroller->context_x.run();
        // 4. 确认工作线程停止
        signal_.reset();
        work.reset();
        for (int i = 0; i < thread_.size(); ++i)
        {
            thread_[i]->join();
            delete thread_[i];
        }
    }
}