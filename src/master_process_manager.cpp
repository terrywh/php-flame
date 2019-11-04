#include "master_process_manager.h"
#include "master_process.h"
#include "coroutine.h"
#include "util.h"

master_process_manager::master_process_manager(boost::asio::io_context& io, unsigned int count, unsigned int close)
: io_(io)
, cmax_(count)
, crun_(0)
, tquit_(close)
, child_(count)
, timer_(io)
, status_(0) {

}

master_process_manager::~master_process_manager() {
    // std::cout << "~master_process_manager\n";
}

void master_process_manager::pm_start(boost::asio::io_context& io) {
    // 这个 work_guard 为了使得主线程在子进程未全部退出前保持运行
    // 实际主线程并没有工作量
    work_.reset(new boost::asio::io_context::work(io));
    for(int i=0;i<cmax_;++i) {
        child_[i].reset(new master_process(io_, this, i));
    }
}

void master_process_manager::pm_reset(coroutine_handler& ch) {
    if(status_ & (STATUS_RSETING | STATUS_ACLOSED)) return;

    std::vector<std::unique_ptr<master_process>> start(cmax_);
    for(int i=0;i<cmax_;++i) {
        start[i].reset(new master_process(io_, this, i));
        ch_start_.reset(ch);
        ch_start_.suspend(); // 等待进程启动回调 on_child_event
        ch_start_.reset();
        // 启动间隔
        util::co_sleep(io_, std::chrono::milliseconds(140), ch);
        // 新进程已经启动 1/4 后，开始停止原进程（一次性动作）
        if(i == cmax_ / 4) coroutine::start(io_.get_executor(), [this] (coroutine_handler ch) {
            pm_close(ch);
        });
    }
    child_ = std::move(start);
}

void master_process_manager::pm_close(coroutine_handler& ch, bool now) {
    if(status_ & (STATUS_RSETING | STATUS_ACLOSED)) return;
    if(status_ & STATUS_CLOSING) now = true; // 已经进行过关闭, 转为强制关闭
    status_ |= STATUS_CLOSING;
    for(int i=0;i<cmax_;++i) child_[i]->close(now);
    if (now) {
        status_ &=~STATUS_TIMEOUT;
        status_ |= STATUS_ACLOSED;
        timer_.cancel(); // 可能有异常重启中的进程存在
        goto SHUTDOWN;
    }
    {
        timer_.expires_after(std::chrono::milliseconds(tquit_));
        timer_.async_wait([&ch, this, self = pm_self()] (const boost::system::error_code& error) mutable {
            if(status_ & STATUS_ACLOSED) return;  // (2) 结束后栈数据丢失（ch)
            status_ |= STATUS_TIMEOUT; // (3) 还未完全关闭时超时或提前强制关闭
            if(ch_close_) ch_close_.resume();
        });
    WAIT_FOR_CLOSE:
        ch_close_.reset(ch);
        ch_close_.suspend(); // 等待回调 on_child_event
        ch_close_.reset();
        if(status_ & STATUS_TIMEOUT) return pm_close(ch, true);
        else if(crun_ > 0) goto WAIT_FOR_CLOSE;
    }
SHUTDOWN:
    status_ |= STATUS_ACLOSED;   // -------> (2)
    timer_.cancel();
    work_.reset();
}

void master_process_manager::pm_kills(int sig) {
    for(int i=0;i<cmax_;++i) child_[i]->signal(sig);
}

void master_process_manager::pm_await() {
    for(int i=0;i<cmax_;++i) child_[i]->await();
}

void master_process_manager::on_child_start(master_process* w) {
    ++crun_;
    if (ch_start_) ch_start_.resume(); // 陆续起停流程
}

void master_process_manager::on_child_close(master_process* w, bool normal) {
    --crun_;
    if (!(status_ & STATUS_CLOSING) && !normal) {
        unsigned int rand = std::rand() % 2000 + 1000;
        output() << "[" << util::system_time() << "] (WARNING) unexpected worker process stopping, restart in " << int(rand/1000) << "s ...\n";
        timer_.expires_after(std::chrono::milliseconds(rand));
        timer_.async_wait([this, self = pm_self(), idx = w->idx_] (const boost::system::error_code& error) {
            if(error) return;
            child_[idx].reset(new master_process(io_, this, idx));
        });
    }
    if (ch_close_) ch_close_.resume(); // 陆续起停流程
    if (crun_ == 0 && normal) work_.reset(); // 正常退出了最后一个进程
    // 这里不太严格：三个进程 1、2 异常退出，3正常退出时也结束了程序
}
