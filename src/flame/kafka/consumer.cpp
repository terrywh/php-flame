#include "../coroutine.h"
#include "../time/time.h"
#include "consumer.h"
#include "_consumer.h"
#include "kafka.h"
#include "message.h"
#include "../../coroutine_queue.h"

namespace flame::kafka {

    void consumer::declare(php::extension_entry& ext) {
        php::class_entry<consumer> class_consumer("flame\\kafka\\consumer");
        class_consumer
            .method<&consumer::__construct>("__construct", {}, php::PRIVATE)
            .method<&consumer::run>("run", {
                {"callable", php::TYPE::CALLABLE},
            })
            .method<&consumer::commit>("commit", {
                {"message", "flame\\kafka\\message"}
            })
            .method<&consumer::close>("close");
        ext.add(std::move(class_consumer));
    }

    php::value consumer::__construct(php::parameters& params) {
        return nullptr;
    }

    php::value consumer::run(php::parameters& params) {
        cb_ = params[0];
        coroutine_handler ch_run {coroutine::current};
        coroutine_queue<php::object> q;
        // 启动若干协程, 然后进行"并行"消费
        int count = cc_;
        for (int i = 0; i < count; ++i) {
            // 启动协程开始消费
            coroutine::start(php::value([this, &ch_run, &q, &count](php::parameters &params) -> php::value {
                coroutine_handler ch {coroutine::current};
                while(true) {
                    try {
                        // consume 本身可能出现异常，不应导致进程停止
                        std::optional<php::object> m = q.pop(ch);
                        if (m) cb_.call({m.value()});
                        else break;
                    } catch(const php::exception& ex) {
                        // 调用用户异常回调
                        gcontroller->call_user_cb("exception", {ex});
                        // 记录错误信息
                        php::object obj = ex;
                        gcontroller->output(0) << "[" << time::iso() << "] (ERROR) Uncaught exception in Kafka consumer: " << obj.call("__toString") << "\n";
                    }
                }
                if (--count == 0) ch_run.resume();
                return nullptr;
            }));
        }
        cs_->consume(q, ch_run);
        ch_run.suspend();
        return nullptr;
    }

    php::value consumer::commit(php::parameters& params) {
        php::object obj = params[0];
        coroutine_handler ch {coroutine::current};

        cs_->commit(obj, ch);
        return nullptr;
    }

    php::value consumer::close(php::parameters& params) {
        coroutine_handler ch {coroutine::current};
        cs_->close(ch);
        return nullptr;
    }
} // namespace
