#include "../controller.h"
#include "../coroutine.h"
#include "tcp.h"
#include "socket.h"
#include "server.h"

namespace flame {
namespace tcp {
	std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
	void declare(php::extension_entry& ext) {
		ext
			.on_module_startup([] (php::extension_entry& ext) -> bool {
				controller_->before([] () { // 在框架初始化后创建
					resolver_.reset(new boost::asio::ip::tcp::resolver(context));
				})->after([] () {
					resolver_.reset();
				});
				return true;
			});
		ext
			.function<connect>("flame\\tcp\\connect", {
				{"address", php::TYPE::STRING},
			});
		socket::declare(ext);
		server::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object o(php::class_entry<socket>::entry());
		socket* o_ = static_cast<socket*>(php::native(o));
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			if(*p == ':') break; // 分离 地址与端口
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "connect tcp socket failed: address port missing");

		std::shared_ptr<coroutine> co = coroutine::current;
		// DNS 地址解析
		resolver_->async_resolve(std::string(s, p - s), std::string(p + 1, e - p - 1), [o, o_, co] (const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type edps) mutable {
			if(error) return co->fail(error);
			// 连接
			boost::asio::async_connect(o_->socket_, edps, [o, o_, co] (const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& edp) mutable {
				if(error) return co->fail(error);
				o.set("local_address", (boost::format("%s:%d") % o_->socket_.local_endpoint().address().to_string() % o_->socket_.local_endpoint().port()).str());
				o.set("remote_address", (boost::format("%s:%d") % o_->socket_.remote_endpoint().address().to_string() % o_->socket_.remote_endpoint().port()).str());
				co->resume(std::move(o));
			});
		});
		return coroutine::async();
	}
}
}