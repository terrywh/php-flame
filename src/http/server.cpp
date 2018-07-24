#include "../coroutine.h"
#include "server.h"
#include "acceptor.h"

namespace flame {
namespace http {
	void server::declare(php::extension_entry& ext) {
		php::class_entry<server> class_server("flame\\http\\server");
		class_server
			.property({"address", "127.0.0.1:7678"})
			.method<&server::__construct>("__construct",{
				{"address", php::TYPE::STRING},
			})
			.method<&server::before>("before", {
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::after>("after", {
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::put>("put", {
				{"path", php::TYPE::STRING},
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::delete_>("delete", {
				{"path", php::TYPE::STRING},
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::post>("post", {
				{"path", php::TYPE::STRING},
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::get>("get", {
				{"path", php::TYPE::STRING},
				{"callback", php::TYPE::CALLABLE},
			})
			.method<&server::run>("run")
			.method<&server::close>("close");
		ext.add(std::move(class_server));
	}
	php::value server::__construct(php::parameters& params) {
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = e-2; p>s; --p) {
			if(*p == ':') break; // 分离 地址与端口
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "create http server failed: address port missing");
		boost::asio::ip::address addr = boost::asio::ip::make_address(std::string(s, p-s));
		addr_.address(addr);
		addr_.port( std::atoi(p + 1) );

		set("address", params[0]);
		return nullptr;
	}
	php::value server::before(php::parameters& params) {
		cb_["before"] = params[0];
		return this;
	}
	php::value server::after(php::parameters& params) {
		cb_["after"] = params[0];
		return this;
	}
	php::value server::put(php::parameters& params) {
		cb_[std::string("PUT:") + params[0].to_string()] = params[1];
		return this;
	}
	php::value server::delete_(php::parameters& params) {
		cb_[std::string("DELETE:") + params[0].to_string()] = params[1];
		return this;
	}
	php::value server::post(php::parameters& params) {
		cb_[std::string("POST:") + params[0].to_string()] = params[1];
		return this;
	}
	php::value server::get(php::parameters& params) {
		cb_[std::string("GET:") + params[0].to_string()] = params[1];
		return this;
	}
	php::value server::run(php::parameters& params) {
		acc_ = std::make_shared<acceptor>(coroutine::current, this);
		acc_->accept();
		return coroutine::async();
	}
	php::value server::close(php::parameters& params) {
		acc_->acceptor_.close();
		cb_.clear(); // 防止由于 cb 进行 use 引用其他对象
		return nullptr;
	}
}
}
