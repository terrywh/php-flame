#include "../coroutine.h"
#include "acceptor.h"
#include "server.h"

namespace flame {
namespace tcp {
	void server::declare(php::extension_entry& ext) {
		php::class_entry<server> class_server("flame\\tcp\\server");
		class_server
			.method<&server::__construct>("__construct", {
				{"bind", php::TYPE::STRING},
			})
			.method<&server::run>("run", {
				{"cb", php::TYPE::CALLABLE},
			})
			.method<&server::close>("close");
		ext.add(std::move(class_server));
	}
	php::value server::__construct(php::parameters& params) {
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			if(*p == ':') break; // 分离 地址与端口
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "create http server failed: address port missing");
		boost::asio::ip::address addr = boost::asio::ip::make_address(std::string(s, p-s));
		addr_.address(addr);
		addr_.port( std::atoi(p + 1) );

		set("address", params[0]);
		return nullptr;
	}
	php::value server::run(php::parameters& params) {
		cb_ = params[0];

		acc_ = std::make_shared<acceptor>(coroutine::current, this);
		acc_->accept();
		return coroutine::async();
	}
	php::value server::close(php::parameters& params) {
		cb_ = nullptr;
		acc_->acceptor_.close();
		return nullptr;
	}
}
}