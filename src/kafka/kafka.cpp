#include "../coroutine.h"
#include "kafka.h"
#include "_consumer.h"
#include "consumer.h"
#include "_producer.h"
#include "producer.h"
#include "message.h"

namespace flame {
namespace kafka {
    void declare(php::extension_entry& ext) {
        ext
			.function<consume>("flame\\kafka\\consume", {
				{"options", php::TYPE::ARRAY},
                {"topics", php::TYPE::ARRAY},
			})
            .function<produce>("flame\\kafka\\produce", {
				{"options", php::TYPE::ARRAY},
                {"topics", php::TYPE::ARRAY},
			});;
		consumer::declare(ext);
		producer::declare(ext);
		message::declare(ext);
    }
    php::value consume(php::parameters& params) {
        php::array config = params[0];
        php::array topics = params[1];
        std::shared_ptr<coroutine> co = coroutine::current;
        // 建立连接
        php::object obj(php::class_entry<consumer>::entry());
        consumer* ptr = static_cast<consumer*>(php::native(obj));
        ptr->cs_.reset(new _consumer(config, topics));
        // 订阅并初始化消费过程
        ptr->cs_->exec([co, ptr] (rd_kafka_t* conn, rd_kafka_resp_err_t& e) -> rd_kafka_message_t* {
            e = ptr->cs_->subscribe_wk();
            return nullptr;
        }, [co, obj] (rd_kafka_t* conn, rd_kafka_message_t* msg, rd_kafka_resp_err_t e) {
            if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                co->fail((boost::format("failed to subscribe topics: %d") % e).str());
            }else{
                co->resume(std::move(obj));
            }
        });
        return coroutine::async();
    }
    php::value produce(php::parameters& params) {
        php::array config = params[0];
        php::array topics = params[1];
        std::shared_ptr<coroutine> co = coroutine::current;

        // 建立连接
        php::object obj(php::class_entry<producer>::entry());
        producer* ptr = static_cast<producer*>(php::native(obj));
        ptr->pd_.reset(new _producer(config, topics));

        return std::move(obj);
    }
    php::array convert(rd_kafka_headers_t* headers) {
        return nullptr;
    }
    rd_kafka_headers_t* convert(php::array headers) {
        return nullptr;
    }
}
}
