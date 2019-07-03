#pragma once
#include "vendor.h"
#include "coroutine.h"

class master_process;
class master_process_manager {
public:
    master_process_manager(boost::asio::io_context& io, unsigned int count);
    virtual ~master_process_manager();
    // 输出
    virtual std::ostream& output() = 0;
protected:
    virtual std::shared_ptr<master_process_manager> pm_self() = 0;
    // 启动子进程
    void pm_start();
    // 重启子进程（陆续）
    void pm_reset(coroutine_handler& ch);
    // 停止子进程
    void pm_close(unsigned int ms, coroutine_handler& ch);
    // 发送信号
    void pm_kills(int sig);
    // 进程数量
    std::uint8_t pm_count() {
        return count_;
    }
    // 进程启动回调
    virtual void on_child_start(master_process* w);
    // 进程停止回调
    virtual void on_child_close(master_process* w, bool normal);
private:
    boost::asio::io_context&                        io_;
    unsigned int                                 count_;
    std::vector<std::unique_ptr<master_process>> child_;
    
    coroutine_handler ch_close_;
    coroutine_handler ch_start_;
    
    friend class master_process;
};
