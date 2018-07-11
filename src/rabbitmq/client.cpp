#include "../coroutine.h"
#include "rabbitmq.h"
#include "client.h"
#include "consumer.h"
#include "producer.h"

namespace flame {
namespace rabbitmq {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\rabbitmq\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::consume>("consume", {
				{"queue", php::TYPE::STRING},
				{"options", php::TYPE::ARRAY, false, true},
			})
			.method<&client::produce>("produce", {
				{"exchange", php::TYPE::STRING, false, true},
				{"options", php::TYPE::ARRAY, false, true},
			});
		ext.add(std::move(class_client));
	}
	php::value client::__construct(php::parameters& params) {
		return nullptr;
	}
	// 创建 consumer
	php::value client::consume(php::parameters& params) {
		php::object c(php::class_entry<consumer>::entry());
		c.set("queue", params[0]);
		consumer* p_ = static_cast<consumer*>(php::native(c));
		p_->flag_ = 0;
		if(params.size() > 1) {
			php::array opts_ = params[1];
			if(opts_.get("nolocal").to_boolean())   p_->flag_ |= AMQP::nolocal;
			if(opts_.get("noack").to_boolean())     p_->flag_ |= AMQP::noack;
			if(opts_.get("exclusive").to_boolean()) p_->flag_ |= AMQP::exclusive;
		}
		p_->amqp_ = amqp_;
		return std::move(c);
	}
	// 创建 producer
	php::value client::produce(php::parameters& params) {
		php::object p(php::class_entry<producer>::entry());
		producer* p_ = static_cast<producer*>(php::native(p));
		std::string ex;
		if(params.size() > 0) {
			ex = params[0].to_string();
		}
		p_->ex_ = ex;
		p.set("exchange", ex);
		p_->flag_ = 0;
		if(params.size() > 1) {
			php::array opts_ = params[1];
			if(opts_.get("mandatory").to_boolean()) p_->flag_ |= AMQP::mandatory;
			if(opts_.get("immediate").to_boolean()) p_->flag_ |= AMQP::immediate;
		}
		p_->amqp_ = amqp_;
		return std::move(p);
	}
}
}
