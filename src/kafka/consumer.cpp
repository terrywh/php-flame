#include "../coroutine.h"
#include "../time/time.h"
#include "consumer.h"
#include "_consumer.h"
#include "kafka.h"
#include "message.h"

namespace flame::kafka {

    void consumer::declare(php::extension_entry& ext) {
		php::class_entry<consumer> class_consumer("flame\\kafka\\consumer");
		class_consumer
			.method<&consumer::__construct>("__construct", {}, php::PRIVATE)
			.method<&consumer::run>("run",
            {
				{"callable", php::TYPE::CALLABLE},
			})
            .method<&consumer::commit>("commit",
            {
                {"message", "flame\\kafka\\message"}
            })
			.method<&consumer::close>("close");
		ext.add(std::move(class_consumer));
	}

	php::value consumer::__construct(php::parameters& params)
    {
		return nullptr;
	}
	php::value consumer::run(php::parameters& params)
    {
        close_ = false;
        ex_ = EG(current_execute_data);
        cb_ = params[0];
        
        // 启动若干协程, 然后进行"并行"消费
        int count = cc_;
        for (int i = 0; i < count; ++i)
        {
            // 启动协程开始消费
            coroutine::start(php::value([this, &count](php::parameters &params) -> php::value {
                coroutine_handler ch {coroutine::current};
                while(!close_)
                {
                    php::object msg = cs_->consume(ch);
                    // 可能出现为 NULL 的情况
                    if (msg.typeof(php::TYPE::NULLABLE))
                    {
                        continue;
                    }
                    try
                    {
                        cb_.call({msg});
                    }
                    catch(const php::exception& ex)
                    {
                        php::object obj = ex;
                        std::cerr << "[" << time::iso() << "] (ERROR) " << obj.call("__toString") << "\n";
                        // std::clog << "[" << time::iso() << "] (ERROR) " << ex.what() << std::endl;
                    }
                }
                if (--count == 0) // 当所有并行协程结束后, 停止消费
                {
                    ch_.resume();
                }
                return nullptr;
            }));
        }
        // 暂停当前消费协程 (等待消费停止)
        ch_.reset(coroutine::current);
        ch_.suspend();
        return nullptr;
	}
    php::value consumer::commit(php::parameters& params)
    {
        php::object obj = params[0];
        coroutine_handler ch {coroutine::current};
        
        cs_->commit(obj, ch);
        return nullptr;
    }
	php::value consumer::close(php::parameters& params) {
        coroutine_handler ch {coroutine::current};
        cs_->close(ch);
        close_ = true;
        return nullptr;
	}
} // namespace 
