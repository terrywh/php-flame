#include "message.h"
#include "kafka.h"
#include "../time/time.h"

namespace flame::kafka {
    void message::declare(php::extension_entry& ext)
    {
        php::class_entry<message> class_message("flame\\kafka\\message");
		class_message
            .implements(&php_json_serializable_ce)
			.property({"topic", ""})
			.property({"partition", -1})
			.property({"key", ""})
            .property({"offset", -1})
            .property({"header", nullptr})
			.property({"payload", nullptr})
			.property({"timestamp", 0})
			.method<&message::__construct>("__construct",
            {
				{"payload", php::TYPE::STRING, false, true},
                {"key", php::TYPE::STRING, false, true},
			})
			.method<&message::to_string>("__toString")
            .method<&message::to_json>("jsonSerialize");

		ext.add(std::move(class_message));
    }
    message::~message()
    {
        if(msg_) rd_kafka_message_destroy(msg_);
    }
    void message::build_ex(rd_kafka_message_t* msg)
    {
        // 用于单挑 message 的提交
        msg_ = msg;
        // ！！！ 是否必须复制出 PHP 的内容？
        set("topic", php::string(rd_kafka_topic_name(msg->rkt)));
        set("partition", msg->partition);
        set("key", php::string((const char*)msg->key, msg->key_len));
        set("offset", msg->offset);
        rd_kafka_headers_t* hdrs;
        if(rd_kafka_message_headers(msg, &hdrs) == RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            set("header", hdrs2array(hdrs));
        }
        // header
        set("payload", php::string((const char*)msg->payload, msg->len));
        set("timestamp", rd_kafka_message_timestamp(msg, nullptr));
    }
    php::value message::__construct(php::parameters& params)
    {
        if(params.size() > 0)
        {
            set("payload", params[0].to_string());
        }
        if(params.size() > 1)
        {
            set("key", params[1].to_string());
        }
        set("header", php::array(4));
        set("timestamp", 
            std::chrono::duration_cast<std::chrono::milliseconds>(flame::time::now().time_since_epoch()).count());
        return nullptr;
    }
    php::value message::to_json(php::parameters& params)
    {
        php::array json(4);
        json.set("topic",  get("topic"));
        json.set("key",    get("key"));
        json.set("payload",get("payload"));
        return json;
    }
    php::value message::to_string(php::parameters& params)
    {
        return get("payload");
    }
} // namespace flame::kafka
