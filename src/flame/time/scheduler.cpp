#include "../controller.h"
#include "scheduler.h"
#include "timer.h"

namespace flame::time {
    scheduler* sched_ = nullptr;

    void scheduler::declare(php::extension_entry& ext) {
        gcontroller
            ->on_init([] (const php::array& options) {
                sched_ = new scheduler();
                sched_->start();
            })
            ->on_stop([] () {
                sched_->close();
                // delete sched_;
            });
    }

    scheduler::scheduler()
    : st_(scheduler::compare)
    , strand_(gcontroller->context_y)
    , tm_(gcontroller->context_y)
    , close_(false) {

    }

    bool scheduler::compare(const std::shared_ptr<timer_base>& tm1, const std::shared_ptr<timer_base>& tm2) {
        return tm2->expire_at() < tm1->expire_at(); // 形成小根堆，堆头为最近需要触发的定时器
    }

    void scheduler::insert(std::shared_ptr<timer_base> tm) {
        // 保证线程安全的堆访问
        boost::asio::post(strand_, [tm, this] () {
            std::shared_ptr<timer_base> front;
            if(!st_.empty()) front = st_.top();
            st_.push(tm);
            // 最高优先级项变更
            if(front != st_.top()) tm_.cancel();
        });
        tm->start();
    }

    void scheduler::remove(std::shared_ptr<timer_base> tm) {
        // 在实际的定时过程中进行删除
        tm->close();
    }

    void scheduler::start() {
        boost::asio::post(strand_, std::bind(&scheduler::run, this));
    }

    void scheduler::close() {
        close_ = true;
        tm_.cancel();
    }

    void scheduler::run() {
NEXT_RUN:
        if(st_.empty()) {
            tm_.expires_after(std::chrono::seconds(1800));
            tm_.async_wait(boost::asio::bind_executor(strand_, [this] (const boost::system::error_code& error) {
                if(!close_) run();
            }));
        }
        else {
            auto tb = st_.top();
            if(tb->closed()) { // 已关闭
                st_.pop();
                goto NEXT_RUN;
            }
            boost::system::error_code error;
            tm_.expires_at(tb->expire_at());
            tm_.async_wait(boost::asio::bind_executor(strand_, [tb, this] (const boost::system::error_code& error) {
                if(error && close_) return;
                else if(error) run(); // 新元素拥有更高优先级
                else {
                    if(tb->repeat_) { // 重复的定时器，重新安排
                        tb->reschedule();
                        st_.pop();
                        st_.push(tb);
                    }
                    // 保证调用发生在主线程
                    boost::asio::post(gcontroller->context_x, std::bind(&timer_base::trigger, tb));
                    run(); // 下一项
                }
            }));
        }
       
        
    }
}
