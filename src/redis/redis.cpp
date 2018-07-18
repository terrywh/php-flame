#include "../coroutine.h"
#include "../dynamic_buffer.h"
#include "redis.h"
#include "_client.h"
#include "_command_base.h"
#include "_command.h"
#include "client.h"
#include "transaction.h"
#include "../tcp/tcp.h"

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
		tcp::resolver_->async_resolve(url->host, std::to_string(url->port ? url->port : 6379),  [url, co, cli] (const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type edps) {
			co->stack(php::value([cli, co] (php::parameters& params) -> php::value {
				return cli;
			}));
			// client* cli_ = static_cast<client*>(php::native(cli));
			auto ptr = std::make_shared<_client>();
			if(std::strlen(url->path) > 1) {
				co->stack(php::value([ptr, co, url] (php::parameters& params) -> php::value {
					php::array arg(1);
					arg[0] = php::string(url->path + 1);
					ptr->send(co, new _command("SELECT", arg));
					return coroutine::async();
				}));
			}
			if(url->pass) {
				std::clog << "9\n";
				co->stack(php::value([ptr, co, url] (php::parameters& params) -> php::value {
					php::array arg(1);
					arg[0] = php::string(url->pass);
					ptr->send(co, new _command("AUTH", arg));
					return coroutine::async();
				}));
			}
			boost::asio::async_connect(ptr->socket_, edps, [ptr, co] (const boost::system::error_code& error, const typename boost::asio::ip::tcp::endpoint& edp) {
				if(error) {
					co->fail(error);
				}else{
					co->resume();
				}
			});
		});
		return coroutine::async();
	}
}
}
