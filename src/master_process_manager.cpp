#include "master_process_manager.h"
#include "master_process.h"
#include "coroutine.h"
#include "util.h"

master_process_manager::master_process_manager(boost::asio::io_context& io, unsigned int count)
: io_(io)
, count_(count) {

}

master_process_manager::~master_process_manager() {
    
}

void master_process_manager::pm_start() {
    for(int i=0;i<count_;++i) {
        child_[i].reset(new master_process(io_, this, i));
    }
}

void master_process_manager::pm_reset(coroutine_handler& ch) {
    std::vector<std::unique_ptr<master_process>> start;
    for(int i=0;i<count_;++i) {
        start[i].reset(new master_process(io_, this, i));
        ch_start_.reset(ch);
        ch_start_.suspend(); // 等待进程启动回调 on_child_event
        ch_start_.reset();
        // 启动间隔
        util::co_sleep(io_, std::chrono::milliseconds(140), ch);
        // 新进程已经启动 1/4 后，开始停止原进程（一次性动作）
        if(i == count_ / 4) coroutine::start(io_.get_executor(), [this] (coroutine_handler ch) {
            pm_close(false, ch);
        });
    }
    child_ = std::move(start);
}

void master_process_manager::pm_close(unsigned int timeout_ms, coroutine_handler& ch) {
    std::vector<std::unique_ptr<master_process>> closing = std::move(child_);
    for(int i=0;i<count_;++i) closing[i]->close(timeout_ms == 0);
    if(timeout_ms == 0) return;

    int  stopped = 0;
    bool timeout = false;
    boost::asio::steady_timer tm(io_);
    tm.expires_after(std::chrono::milliseconds(timeout_ms));
    tm.async_wait([&ch, &timeout, this] (const boost::system::error_code& error) mutable {
        if(error) return;
        timeout = true;
        ch.resume();
    });
WAIT_FOR_CLOSE:
    ch_close_.reset(ch);
    ch_close_.suspend(); // 等待回调 on_child_event
    ch_close_.reset();

    if(timeout) {
        child_ = std::move(closing);
        pm_close(0, ch);
        return;
    }
    if(++stopped < count_) goto WAIT_FOR_CLOSE;
    else tm.cancel();
}

void master_process_manager::pm_kills(int sig) {
    for(int i=0;i<count_;++i) child_[i]->signal(sig);
}

void master_process_manager::on_child_start(master_process* w) {
    if (ch_start_) ch_start_.resume(); // 陆续起停流程
}

void master_process_manager::on_child_close(master_process* w, bool normal) {
    if (ch_close_) ch_close_.resume(); // 陆续起停流程
}