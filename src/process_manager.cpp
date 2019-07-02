#include "process_manager.h"
#include "coroutine.h"
#include "util.h"

process_manager::process_manager(boost::asio::io_context& io, unsigned int count)
: io_(io)
, count_(count) {

}

void process_manager::on_child_start(process_child* w) {

}

void process_manager::on_child_close(process_child* w, bool normal) {

}

void process_manager::start() {
    for(int i=0;i<count_;++i) {
        child_[i].reset(new process_child(io_, this, i));
    }
}

void process_manager::restart(coroutine_handler& ch) {
    std::vector<std::unique_ptr<process_child>> start;
    for(int i=0;i<count_;++i) {
        start[i].reset(new process_child(io_, this, i));
        ch_start_.reset(ch);
        ch_start_.suspend(); // 等待进程启动回调 on_child_event
        ch_start_.reset();
        // 启动间隔
        util::co_sleep(io_, std::chrono::milliseconds(140), ch);
        // 新进程已经启动 1/4 后，开始停止原进程（一次性动作）
        if(i == count_ / 4) coroutine::start(io_.get_executor(), [this] (coroutine_handler ch) {
            close(false, ch);
        });
    }
    child_ = std::move(start);
}

void process_manager::close(unsigned int timeout_ms, coroutine_handler& ch) {
    std::vector<std::unique_ptr<process_child>> closing = std::move(child_);
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
        close(0, ch);
        return;
    }
    if(++stopped < count_) goto WAIT_FOR_CLOSE;
    else tm.cancel();
}


void process_manager::signal(int sig) {
    for(int i=0;i<count_;++i) child_[i]->signal(sig);
}
