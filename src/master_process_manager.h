#pragma once
#include "vendor.h"
#include "coroutine.h"

class master_process;
class master_process_manager {
public:
    master_process_manager(boost::asio::io_context& io, unsigned int count, unsigned int timeout_ms);
    virtual ~master_process_manager();
    // 输出
    virtual std::ostream& output() = 0;
protected:
    virtual std::shared_ptr<master_process_manager> pm_self() = 0;
    // 启动子进程
    void pm_start(boost::asio::io_context& io); // 此 io 非彼 io
    // 重启子进程（陆续）
    void pm_reset(coroutine_handler& ch);
    // 停止子进程
    void pm_close(coroutine_handler& ch, bool now = false);
    // 发送信号
    void pm_kills(int sig);
    // 进程数量
    std::uint8_t pm_count() {
        return cmax_;
    }
protected:
    // 进程启动回调
    virtual void on_child_start(master_process* w);
    // 进程停止回调
    virtual void on_child_close(master_process* w, bool normal);
private:
    std::unique_ptr<boost::asio::io_context::work> work_;
    boost::asio::io_context&                        io_;
    unsigned int                                 cmax_;
    unsigned int                                 crun_;
    unsigned int                                 tquit_;

    std::vector<std::unique_ptr<master_process>> child_;
    boost::asio::steady_timer                    timer_;
    coroutine_handler ch_close_;
    coroutine_handler ch_start_;
    int status_;
    
    enum {
        STATUS_CLOSING = 0x01,
        STATUS_RSETING = 0x02,
        STATUS_QUITING = 0x04,
        STATUS_ACLOSED = 0x08,
        STATUS_TIMEOUT = 0x10,
    };
    
    friend class master_process;
};
