#include "../coroutine.h"
#include "../time/time.h"
#include "consumer.h"
#include "_consumer.h"
#include "kafka.h"
#include "message.h"
#include "../../coroutine_queue.h"
#include "../log/logger.h"

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
        // close_ = false;
        return nullptr;
    }

    php::value consumer::run(php::parameters& params) {
        cb_ = params[0];
        coroutine_handler ch_run {coroutine::current};
        coroutine_queue<rd_kafka_message_t*> q;
        // 启动若干协程, 然后进行"并行"消费
        int count = cc_;
        for (int i = 0; i < count; ++i) {
            // 启动协程开始消费
            coroutine::start(php::value([this, &ch_run, &q, &count](php::parameters &params) -> php::value {
                coroutine_handler ch {coroutine::current};
                while(true) {
                    try {
                        // consume 本身可能出现异常，不应导致进程停止
                        std::optional<rd_kafka_message_t*> m = q.pop(ch);
                        if (m) {
                            php::object obj(php::class_entry<message>::entry());
                            message* ptr = static_cast<message*>(php::native(obj));
                            ptr->build_ex(m.value()); // msg 交由 message 对象管理
                            cb_.call({obj});
                        }
                        else break;
                    } catch(const php::exception& ex) {
                        // 调用用户异常回调
                        gcontroller->event("exception", {ex});
                        // 记录错误信息
                        php::object obj = ex;
                        log::logger_->stream() << "[" << time::iso() << "] (ERROR) Uncaught exception in Kafka consumer: " << obj.call("__toString") << std::endl;
                    }
                }
                // 最后一个消费协程停止后，回复 run() 阻塞的协程
                if (--count == 0) ch_run.resume(); // (B) -> (C)
                return nullptr;
            }));
        }
        // boost::asio::io_context::strand consumer_strand {gcontroller->context_y};
        // 启动轻量的 C++ 协程进行消费
        ::coroutine::start(gcontroller->context_y.get_executor(), [this, &q] (::coroutine_handler ch) {
            boost::asio::steady_timer tm { gcontroller->context_y };
            while(!close_) {
                if(!cs_->consume(q, ch)) { // 消费到结尾后，适当等待
                    tm.expires_from_now(std::chrono::milliseconds(40));
                    tm.async_wait(ch);
                }
            }
            cs_->close();
            q.close(); // (A) -> (B)
        });
        ch_run.suspend(); // (C)
        return nullptr;
    }

    php::value consumer::commit(php::parameters& params) {
        php::object obj = params[0];
        coroutine_handler ch {coroutine::current};

        cs_->commit(obj, ch);
        return nullptr;
    }

    php::value consumer::close(php::parameters& params) {
        close_ = true;
        return nullptr;
    }
} // namespace
