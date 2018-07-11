#include "../coroutine.h"
#include "rabbitmq.h"
#include "producer.h"
#include "message.h"

namespace flame {
namespace rabbitmq {
	void producer::declare(php::extension_entry& ext) {
		php::class_entry<producer> class_producer("flame\\rabbitmq\\producer");
		class_producer
			.property({"exchange", ""})
			.method<&producer::__construct>("__construct", {}, php::PRIVATE)
			.method<&producer::publish>("publish", {
				{"message", php::TYPE::UNDEFINED},
			});
		ext.add(std::move(class_producer));
	}
	php::value producer::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value producer::publish(php::parameters& params) {
		std::string routing_key;
		if(params.size() > 1) {
			routing_key = params[1].to_string();
		}
		if(params[0].instanceof(php::class_entry<message>::entry())) {
			php::object msg = params[0];
			message* msg_ = static_cast<message*>(php::native(msg));
			php::string body = msg.get("body");
			body.to_string();
			AMQP::Envelope env(body.c_str(), body.size());
			msg_->build_ex(env);
			amqp_.channel->publish(ex_, routing_key, env, flag_);
		}else{
			php::string msg = params[0];
			msg.to_string();
			amqp_.channel->publish(ex_, routing_key, msg.c_str(), msg.size(), flag_);
		}
		return nullptr;
	}
}
}
