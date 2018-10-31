#include "../coroutine.h"
#include "rabbitmq.h"
#include "consumer.h"
#include "producer.h"
#include "message.h"

namespace flame {
namespace rabbitmq {
	void declare(php::extension_entry& ext) {
		ext
			.function<consume>("flame\\rabbitmq\\consume", {
				{"address", php::TYPE::STRING},
				{"queue", php::TYPE::STRING},
				{"options", php::TYPE::ARRAY, false, true},
			})
			.function<produce>("flame\\rabbitmq\\produce", {
				{"address", php::TYPE::STRING},
				{"options", php::TYPE::ARRAY, false, true},
			});
		consumer::declare(ext);
		producer::declare(ext);
		message::declare(ext);
	}
	client_context::client_context(const php::string& addr)
	: handler(context)
	, connection(&handler, AMQP::Address(addr.c_str(), addr.size()))
	, channel(&connection) {

	}
	static void connect(std::shared_ptr<client_context> cc, std::shared_ptr<coroutine> co,
						const php::string& addr, std::uint16_t prefetch) {
		cc->channel.setQos(prefetch);
		cc->channel.onReady([cc, co] () mutable {
			co->resume();
			co.reset();
			cc->channel.onError(nullptr);
		});
		cc->channel.onError([cc, co] (const char* message) mutable {
			co->fail(message);
			co.reset();
			cc->channel.onReady(nullptr);
		});
	}
	php::value consume(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		php::string addr  = params[0];
		std::string queue = params[1];
		std::uint16_t prefetch = 8;
		int           flag  = 0;
		if(params.size() > 2) {
			php::array  opts  = params[2];
			if(opts.get("nolocal").to_boolean())   flag |= AMQP::nolocal;
			if(opts.get("noack").to_boolean())     flag |= AMQP::noack;
			if(opts.get("exclusive").to_boolean()) flag |= AMQP::exclusive;
			if(opts.exists("prefetch")) {
				prefetch = std::max(std::uint16_t(1), std::min(std::uint16_t(opts.get("prefetch").to_integer()), std::uint16_t(65535)));
			}
		}

		std::shared_ptr<client_context> cc = std::make_shared<client_context>(addr);
		co->stack(php::value([co, cc, queue, flag] (php::parameters& params) -> php::value {
			php::object cobj(php::class_entry<consumer>::entry());
			consumer* cptr = static_cast<consumer*>(php::native(cobj));
			cptr->amqp_  = cc;
			cptr->flag_  = flag;
			cptr->queue_ = queue;
			co->resume(std::move(cobj));
			return nullptr;
		}));
		connect(cc, co, addr, prefetch);
		return coroutine::async();
	}
	php::value produce(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		php::string addr  = params[0];
		int         flag  = 0;
		std::uint16_t prefetch = 8;
		if(params.size() > 1) {
			php::array  opts  = params[1];
			if(opts.get("mandatory").to_boolean()) flag |= AMQP::mandatory;
			if(opts.get("immediate").to_boolean()) flag |= AMQP::immediate;
			if(opts.exists("prefetch")) {
				prefetch = std::max(std::uint16_t(1), std::min(std::uint16_t(opts.get("prefetch").to_integer()), std::uint16_t(65535)));
			}
		}

		std::shared_ptr<client_context> cc = std::make_shared<client_context>(addr);
		co->stack(php::value([co, cc, flag] (php::parameters& params) -> php::value {
			php::object pobj(php::class_entry<producer>::entry());
			producer*   pptr = static_cast<producer*>(php::native(pobj));
			pptr->amqp_ = cc;
			pptr->flag_ = flag;
			co->resume(std::move(pobj));
			return nullptr;
		}));
		connect(cc, co, addr, prefetch);
		return coroutine::async();
	}
	// 仅支持一维数组
	php::array convert(const AMQP::Table& table) {
		php::array data(table.size());
		for(auto key : table.keys()) {
			const AMQP::Field& field = table.get(key);
			if(field.isBoolean()) {
				data.set(key, static_cast<uint8_t>(field) ? true : false);
			}else if(field.isInteger()) {
				data.set(key, static_cast<int64_t>(field));
			}else if(field.isDecimal()) {
				data.set(key, static_cast<double>(field));
			}else if(field.isString()) {
				data.set(key, (const std::string&)field);
			}else if(field.typeID() == 'd') {
				data.set(key, static_cast<double>(field));
			}else if(field.typeID() == 'f') {
				data.set(key, static_cast<float>(field));
			}
			// TODO 记录警告信息?
		}
		return std::move(data);
	}
	// 仅支持一维数组
	AMQP::Table convert(const php::array& table) {
		AMQP::Table data;
		for(auto i=table.begin(); i!=table.end(); ++i) {
			if(i->second.typeof(php::TYPE::BOOLEAN)) {
				data.set(i->first.to_string(), i->second.to_boolean());
			}else if(i->second.typeof(php::TYPE::INTEGER)) {
				data.set(i->first.to_string(), i->second.to_integer());
			}else if(i->second.typeof(php::TYPE::STRING)) {
				data.set(i->first.to_string(), i->second.to_string());
			}else if(i->second.typeof(php::TYPE::FLOAT)) {
				data.set(i->first.to_string(), AMQP::Double {i->second.to_float()});
			}else{
				throw php::exception(zend_ce_type_error, "table conversion failed: unsupported type");
			}
		}
		return std::move(data);
	}
}
}
