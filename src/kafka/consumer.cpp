#include "../coroutine.h"
#include "kafka.h"
#include "consumer.h"
#include "_consumer.h"
#include "message.h"

namespace flame {
namespace kafka {

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
    void consumer::consume() {
        if(close_) {
            std::shared_ptr<coroutine> co = co_;
            co_.reset();
            cb_ = nullptr;

            co->resume();
            return;
        }
        cs_->exec([this] (rd_kafka_t* conn, rd_kafka_resp_err_t& e) -> rd_kafka_message_t* {
            rd_kafka_message_t* msg = rd_kafka_consumer_poll(conn, 1000);
            if(!msg) {
                e = RD_KAFKA_RESP_ERR__PARTITION_EOF;
                return nullptr;
            }else if(msg->err) {
                e = msg->err;
                rd_kafka_message_destroy(msg);
                return nullptr;
            }else{
                return msg;
            }
        }, [this] (rd_kafka_t* conn, rd_kafka_message_t* msg, rd_kafka_resp_err_t e) {
            if(e == RD_KAFKA_RESP_ERR__PARTITION_EOF ) {
                consume();
            }else if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                close_ = true;
                co_->fail((boost::format("failed to poll for message: %d") % e).str());
                co_.reset();
                cb_= nullptr;
            }else if(msg) {
                php::object obj(php::class_entry<message>::entry());
                message* ptr = static_cast<message*>(php::native(obj));
                ptr->build_ex(cs_, msg);

                // TODO 能否复用协程对象？
                std::make_shared<coroutine>()->start(cb_, {std::move(obj)});
                consume();
            }else{
                consume();
            }
        });
    }
	php::value consumer::run(php::parameters& params) {
        close_ = false;
        co_ = coroutine::current;
        cb_ = params[0];
        co_->stack(nullptr, php::object(this));
        consume();
		return coroutine::async();
	}
    php::value consumer::commit(php::parameters& params) {
        std::shared_ptr<coroutine> co = coroutine::current;
        php::object obj = params[0];
        co->stack(nullptr, php::object(this));
        rd_kafka_message_t* msg = static_cast<message*>(php::native(obj))->msg_;
        cs_->exec([obj, msg] (rd_kafka_t* conn, rd_kafka_resp_err_t& e) -> rd_kafka_message_t* {
            e = rd_kafka_commit_message(conn, msg, 0);
            return nullptr;
        }, [co] (rd_kafka_t* conn, rd_kafka_message_t* msg, rd_kafka_resp_err_t e) {
            if(e != RD_KAFKA_RESP_ERR_NO_ERROR) {
                co->fail((boost::format("failed to commit kafka message: %d") % e).str());
            }else{
                co->resume();
            }
        });
        return coroutine::async();
    }
	php::value consumer::close(php::parameters& params) {
        if(!close_ && co_ && !cb_.empty()) {
            close_ = true;
        }
        return nullptr;
	}
}
}
