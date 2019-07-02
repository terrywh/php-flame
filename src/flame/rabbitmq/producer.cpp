#include "../coroutine.h"
#include "producer.h"
#include "_client.h"
#include "message.h"

namespace flame::rabbitmq {

    void producer::declare(php::extension_entry &ext) {
        php::class_entry<producer> class_producer("flame\\rabbitmq\\producer");
        class_producer
            .method<&producer::__construct>("__construct", {}, php::PRIVATE)
            .method<&producer::publish>("publish", {
                {"exchange", php::TYPE::STRING},
                {"message", php::TYPE::UNDEFINED},
            });
        ext.add(std::move(class_producer));
    }

    php::value producer::__construct(php::parameters &params) {
        return nullptr;
    }

    php::value producer::publish(php::parameters &params) {
        std::string exch = params[0];
        std::string routing_key;
        if (params.size() > 2) routing_key = params[2].to_string();
        
        coroutine_handler ch{coroutine::current};
        if (params[1].instanceof(php::class_entry<message>::entry())) {
            php::object msg = params[1];
            message *msg_ = static_cast<message *>(php::native(msg));
            php::string body = msg.get("body");
            body.to_string();
            AMQP::Envelope env(body.c_str(), body.size());
            msg_->build_ex(env);
            // 未指定时使用 message 默认的 routing_key
            if (routing_key.empty()) routing_key = msg.get("routing_key").to_string();

            coroutine_handler ch {coroutine::current};
            cc_->publish(exch, routing_key, env, ch);
        }
        else {
            php::string msg = params[1];
            msg.to_string();
            cc_->publish(exch, routing_key, msg.c_str(), msg.size(), ch);
        }
        return nullptr;
    }
} // namespace flame::rabbitmq
