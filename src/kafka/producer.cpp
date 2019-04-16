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
            .method<&producer::flush>("flush");
        ext.add(std::move(class_producer));
    }

    php::value producer::__construct(php::parameters &params) {
        return nullptr;
    }

    php::value producer::__destruct(php::parameters &params) {
        if (pd_) {
            coroutine_handler ch {coroutine::current};
            pd_->flush(ch);
        }
        return nullptr;
    }

    php::value producer::publish(php::parameters &params) {
        php::string topic = params[0].to_string(), key, payload;
        php::array header;
        if (params[1].instanceof (php::class_entry<message>::entry())) {
            php::object msg = params[1];
            key = msg.get("key").to_string();
            payload = msg.get("payload").to_string();
            header = msg.get("header");
            if (!header.type_of(php::TYPE::ARRAY)) header = php::array(0);
        }
        else {
            payload = params[1].to_string();
            if (params.size() > 2) key = params[2].to_string();
            else key = php::string(0);
            if (params.size() > 3) {
                if (params[3].type_of(php::TYPE::ARRAY)) header = params[4];
                else header = php::array(0);
            }
        }

        coroutine_handler ch {coroutine::current};
        pd_->publish(topic, key, payload, header, ch);
        return nullptr;
    }
    
    php::value producer::flush(php::parameters &params) {
        coroutine_handler ch {coroutine::current};
        pd_->flush(ch);
        return nullptr;
    }
} // namespace flame::kafka
