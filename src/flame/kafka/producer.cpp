#include "../coroutine.h"
#include "kafka.h"
#include "message.h"
#include "_producer.h"
#include "producer.h"

namespace flame::kafka {

    void producer::declare(php::extension_entry &ext) {
        php::class_entry<producer> class_producer("flame\\kafka\\producer");
        class_producer
            .method<&producer::__construct>("__construct", {}, php::PRIVATE)
            .method<&producer::__destruct>("__destruct")
            .method<&producer::publish>("publish", {
                {"topic", php::TYPE::STRING},
            })
            .method<&producer::flush>("flush")
            .method<&producer::close>("close");
        ext.add(std::move(class_producer));
    }

    php::value producer::__construct(php::parameters &params) {
        return nullptr;
    }

    php::value producer::__destruct(php::parameters &params) {
        if (pd_) pd_->close(); // 内部存在 flush 机制
        return nullptr;
    }

    php::value producer::publish(php::parameters& params) {
        php::string topic = params[0].to_string(), key(0), payload;
        php::array header;
        std::int32_t p = RD_KAFKA_PARTITION_UA, i = 1;
        // 目标分区
        if (params[1].type_of(php::TYPE::INTEGER)) {
            p = params[1].to_integer();
            ++i;
        }
        if (params[i].instanceof (php::class_entry<message>::entry())) {
            php::object msg = params[i];
            key = msg.get("key").to_string();
            payload = msg.get("payload").to_string();
            header = msg.get("header");
            if (!header.type_of(php::TYPE::ARRAY)) header = php::array(0);
        }
        else {
            payload = params[i].to_string();
            if (params.size() > i+1) key = params[i+1].to_string();
            else key = php::string(0);
            if (params.size() > i+2) {
                if (params[i+2].type_of(php::TYPE::ARRAY)) header = params[i+2];
                else header = php::array(0);
            }
        }

        coroutine_handler ch {coroutine::current};
        pd_->publish(topic, p, key, payload, header, ch);
        return nullptr;
    }

    php::value producer::flush(php::parameters &params) {
        coroutine_handler ch {coroutine::current};
        pd_->flush(ch);
        return nullptr;
    }

    php::value producer::close(php::parameters &params) {
        pd_->close();
        pd_.reset();
        return nullptr;
    }
} // namespace flame::kafka
