#include "../coroutine.h"
#include "kafka.h"
#include "message.h"
#include "_producer.h"
#include "producer.h"

namespace flame {
namespace kafka {
    void producer::declare(php::extension_entry& ext) {
        php::class_entry<producer> class_producer("flame\\kafka\\producer");
		class_producer
			.method<&producer::__construct>("__construct", {}, php::PRIVATE)
            .method<&producer::__destruct>("__destruct")
			.method<&producer::publish>("publish", {
				{"topic", php::TYPE::STRING},
			})
            .method<&producer::flush>("flush");
		ext.add(std::move(class_producer));
    }
    php::value producer::__construct(php::parameters& params) {
        return nullptr;
    }
    php::value producer::__destruct(php::parameters& params) {
        std::shared_ptr<_producer> pd = pd_;
        pd_->exec([pd] (rd_kafka_t* conn) -> rd_kafka_resp_err_t {
            return rd_kafka_flush(conn, 10000);
        }, [] (rd_kafka_t* conn, rd_kafka_resp_err_t  e) {
            // 所有数据项需要回到主线程后才能销毁
            if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                std::cerr << boost::format("failed to flush kafka producer: %d") % e;
            }
        });
        return nullptr;
    }
    php::value producer::publish(php::parameters& params) {
        php::string topic = params[0].to_string(), key, payload;
        php::array header;
        if(params[1].instanceof(php::class_entry<message>::entry())) {
            php::object msg = params[1];
            key = msg.get("key").to_string();
            payload = msg.get("payload").to_string();
            header = msg.get("header");
            if(!header.typeof(php::TYPE::ARRAY)) header = php::array(0);
        }else{
            payload = params[1].to_string();

            if(params.size() > 2) {
                key = params[2].to_string();
            }else{
                key = php::string(0);
            }
            if(params.size() > 3) {
                if(params[3].typeof(php::TYPE::ARRAY)) header = params[4];
                else header = php::array(0);
            }
        }

        rd_kafka_headers_t* hdrs = nullptr;
        if(header.size() > 0) {
            hdrs = rd_kafka_headers_new(header.size());
            for(auto i=header.begin();i!=header.end();++i) {
                php::string key = i->first.to_string(),
                    val = i->second.to_string();
                rd_kafka_header_add(hdrs,
                    key.c_str(), key.size(),
                    val.c_str(), val.size());
            }
        }
        std::shared_ptr<coroutine> co = coroutine::current;
        co->stack(nullptr, php::object(this));
        pd_->exec([this, topic, key, payload, hdrs] (rd_kafka_t* conn) -> rd_kafka_resp_err_t {
            return pd_->publish_wk(topic, key, payload, hdrs);
        }, [topic, key, payload, hdrs, co] (rd_kafka_t* conn, rd_kafka_resp_err_t  e) {
            // 所有数据项需要回到主线程后才能销毁
            if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                if(hdrs) rd_kafka_headers_destroy(hdrs); // 发生错误时须销毁
                co->fail((boost::format("failed to publish kafka message: %d") % e).str());
            }else{
                co->resume();
            }
        });
        return coroutine::async();
    }
    php::value producer::flush(php::parameters& params) {
        std::shared_ptr<coroutine> co = coroutine::current;
        co->stack(nullptr, php::object(this));
        pd_->exec([] (rd_kafka_t* conn) -> rd_kafka_resp_err_t {
            return rd_kafka_flush(conn, 10000);
        }, [co] (rd_kafka_t* conn, rd_kafka_resp_err_t  e) {
            // 所有数据项需要回到主线程后才能销毁
            if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                co->fail((boost::format("failed to flush kafka producer: %d") % e).str());
            }else{
                co->resume();
            }
        });
        return coroutine::async();
    }
}
}
