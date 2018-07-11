#include "../coroutine.h"
#include "client.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "executor.h"

namespace flame {
namespace http {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\http\\client");
		class_client
			.property({"connection_per_host", 4})
			.method<&client::__construct>("__construct", {
				{"options", php::TYPE::ARRAY, false, true},
			})
			.method<&client::exec>("exec", {
				{"options", "flame\\http\\client_request"},
			})
			.method<&client::get>("get", {
				{"url", php::TYPE::STRING},
				{"timeout", php::TYPE::INTEGER, false, true}
			})
			.method<&client::post>("post", {
				{"url", php::TYPE::STRING},
				{"body", php::TYPE::INTEGER},
				{"timeout", php::TYPE::INTEGER, false, true}
			})
			.method<&client::post>("put", {
				{"url", php::TYPE::STRING},
				{"body", php::TYPE::INTEGER},
				{"timeout", php::TYPE::INTEGER, false, true}
			})
			.method<&client::post>("delete", {
				{"url", php::TYPE::STRING},
				{"timeout", php::TYPE::INTEGER, false, true}
			});
		ext.add(std::move(class_client));
	}
	client::client()
	: resolver_(context)
	, context_(ssl::context::sslv23) {
		// context_.set_options(ssl::context::no_sslv2);
		context_.set_verify_mode(ssl::verify_none);
	}
	php::value client::__construct(php::parameters& params) {
		if(params.size() > 0) {
			php::array opts = params[0];
			if(opts.exists("connection_per_host")) {
				int host = opts.get("conn_per_host");
				if(host > 0 && host < 512) {
					
				}else{
					
				}
			}
		}
		return nullptr;
	}
	php::value client::exec_ex(const php::object& req) {
		php::string url = req.get("url");
		if(!url.typeof(php::TYPE::STRING)) {
			throw php::exception(zend_ce_exception, "request 'url' must be a string");
		}
		
		if(url.size() > 8 && strncasecmp(url.c_str(), "https://", 8) == 0) {
			std::shared_ptr<executor<ssl::stream<tcp::socket>>> e(new executor<ssl::stream<tcp::socket>>( &obj_, req )) ;
			e->execute();
		}else if(url.size() > 7 && strncasecmp(url.c_str(), "http://", 7) == 0) {
			std::shared_ptr<executor<tcp::socket>> e(new executor<tcp::socket>(&obj_, req));
			e->execute();
		}else{
			throw php::exception(zend_ce_exception, "request protocol not supported");
		}
		return coroutine::async();
	}
	php::value client::exec(php::parameters& params) {
		return exec_ex(params[0]);
	}
	php::value client::get(php::parameters& params) {
		php::object req(php::class_entry<client_request>::entry());
		req.set("method", "GET");
		req.set("url", params[0]);
		req.set("header", php::array(0));
		req.set("cookie", php::array(0));
		req.set("body", nullptr);
		if(params.length() > 1) {
			req.set("timeout", params[1]);
		}else{
			req.set("timeout", 3000);
		}
		return exec_ex(req);
	}
	php::value client::post(php::parameters& params) {
		php::object req(php::class_entry<client_request>::entry());
		req.set("method",        "POST");
		req.set("url",        params[0]);
		req.set("header", php::array(0));
		req.set("cookie", php::array(0));
		req.set("body",       params[1]);
		if(params.length() > 2) {
			req.set("timeout",params[2]);
		}else{
			req.set("timeout",     3000);
		}
		return exec_ex(req);
	}
	php::value client::put(php::parameters& params) {
		php::object req(php::class_entry<client_request>::entry());
		req.set("method",        "PUT");
		req.set("url",        params[0]);
		req.set("header", php::array(0));
		req.set("cookie", php::array(0));
		req.set("body",       params[1]);
		if(params.length() > 2) {
			req.set("timeout",params[2]);
		}else{
			req.set("timeout",     3000);
		}
		return exec_ex(req);
	}
	php::value client::delete_(php::parameters& params) {
		php::object req(php::class_entry<client_request>::entry());
		req.set("method",         "GET");
		req.set("url",        params[0]);
		req.set("header", php::array(0));
		req.set("cookie", php::array(0));
		req.set("body",         nullptr);
		if(params.length() > 1) {
			req.set("timeout", params[1]);
		}else{
			req.set("timeout", 3000);
		}
		return exec_ex(req);
	}

}
}
