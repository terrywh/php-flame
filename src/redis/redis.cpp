#include "../coroutine.h"
#include "../dynamic_buffer.h"
#include "redis.h"
#include "_client.h"
#include "_command_base.h"
#include "_command.h"
#include "client.h"
#include "transaction.h"

namespace flame {
namespace redis {
	
	void declare(php::extension_entry& ext) {
		ext.function<connect>("flame\\redis\\connect", {
			{"uri", php::TYPE::STRING}
		});
		client::declare(ext);
		transaction::declare(ext);
	}
	php::value connect(php::parameters& params) {
		std::shared_ptr<php::url> url = php::parse_url(params[0]);
		std::shared_ptr<coroutine> co = coroutine::current;
		
		php::object cli(php::class_entry<client>::entry());
		co->stack(php::value([cli, co] (php::parameters& params) -> php::value {
			return cli;
		}));
		static_cast<client*>(php::native(cli))->cli_->connect(coroutine::current, url);
		return coroutine::async();
	}
}
}