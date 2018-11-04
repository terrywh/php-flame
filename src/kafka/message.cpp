#include "../time/time.h"
#include "kafka.h"
#include "message.h"
#include "_consumer.h"

namespace flame {
namespace kafka {
    void message::declare(php::extension_entry& ext) {
        php::class_entry<message> class_message("flame\\kafka\\message");
		class_message
			.property({"topic", ""})
			.property({"partition", -1})
			.property({"key", ""})
            .property({"offset", -1})
            .property({"header", nullptr})
			.property({"payload", nullptr})
			.property({"timestamp", 0})
			.method<&message::__construct>("__construct", {
				{"payload", php::TYPE::STRING, false, true},
                {"key", php::TYPE::STRING, false, true},
			})
			.method<&message::to_string>("__toString")
            .method<&message::to_json>("jsonSerialize");

		ext.add(std::move(class_message));
    }
    message::message()
    : msg_(nullptr) {

    }
    void message::build_ex(std::shared_ptr<_consumer> cs, rd_kafka_message_t* msg) {
        cs_  = cs;
        msg_ = msg;
        // ！！！ 是否必须复制出 PHP 的内容？
        set("topic", php::string(rd_kafka_topic_name(msg->rkt)));
        set("partition", msg->partition);
        set("key", php::string((const char*)msg->key, msg->key_len));
        set("offset", msg->offset);
        rd_kafka_headers_t* hdrs;
        if(rd_kafka_message_headers(msg, &hdrs) == RD_KAFKA_RESP_ERR_NO_ERROR) {
            php::array header(rd_kafka_header_cnt(hdrs));
            const char* key;
            const void* val;
            std::size_t len;
            for(std::size_t i=0; rd_kafka_header_get_all(hdrs, i, &key, &val, &len) == RD_KAFKA_RESP_ERR_NO_ERROR; ++i) {
                header.set(key, php::string((const char*)val, len));
            }
            set("header", header);
        }
        // header
        set("payload", php::string((const char*)msg->payload, msg->len));
        set("timestamp", rd_kafka_message_timestamp(msg, nullptr));
    }
    php::value message::__construct(php::parameters& params) {
        if(params.size() > 0) {
            set("payload", params[0].to_string());
        }
        if(params.size() > 1) {
            set("key", params[1].to_string());
        }
        set("header", php::array(4));
        set("timestamp", flame::time::now());
        return nullptr;
    }
    php::value message::__destruct(php::parameters& params) {
        // 生产者会接管 msg 对象的释放
        if(msg_) {
            std::shared_ptr<_consumer> cs = cs_;
            rd_kafka_message_t* msg = msg_;
            cs_->exec([cs, msg] (rd_kafka_t* conn, rd_kafka_resp_err_t& e) -> rd_kafka_message_t* {
                rd_kafka_message_destroy(msg);
                return nullptr;
            }, [] (rd_kafka_t* conn, rd_kafka_message_t* msg, rd_kafka_resp_err_t  e) {

            });
        }
        return nullptr;
    }
    php::value message::to_json(php::parameters& params) {
        php::array json(4);
        json.set("topic",  get("topic"));
        json.set("header", get("header"));
        json.set("key",    get("key"));
        json.set("payload",get("payload"));
        return json;
    }
    php::value message::to_string(php::parameters& params) {
        return get("payload");
    }
}
}
