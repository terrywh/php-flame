#include "../coroutine.h"
#include "value_body.h"
#include "client_request.h"
#include "http.h"

namespace flame {
namespace http {
	void client_request::declare(php::extension_entry& ext) {
		php::class_entry<client_request> class_client_request(
			"flame\\http\\client_request");
		class_client_request
			.property({"timeout", 3000})
			.property({"method", "GET"})
			.property({"url", nullptr})
			.property({"header", nullptr})
			.property({"cookie", nullptr})
			.property({"body", ""})
			.method<&client_request::__construct>("__construct", {
				{"url", php::TYPE::STRING},
				{"body", php::TYPE::UNDEFINED, false, true},
				{"timeout", php::TYPE::INTEGER, false, true},
			});

		ext.add(std::move(class_client_request));
	}
	php::value client_request::__construct(php::parameters& params) {
		if (params.length() > 2) {
			set("timeout", params[2]);
		}else{
			set("timeout", 3000);
		}
		if (params.length() > 1 && !params[1].empty()) {
			set("body", params[1]);
			set("method", "POST");
		}else{
			set("method", "GET");
		}
		set("url", params[0]);
		set("header", php::array(0));
		set("cookie", php::array(0));
		return nullptr;
	}
	void client_request::build_url()
	{
		if(url_) return; // 防止重复 build
		// 目标请求地址
        php::string u = get("url", true);
		if(!u.typeof(php::TYPE::STRING)) {
			throw php::exception(zend_ce_exception, "request 'url' must be a string");
		}

		url_ = std::make_shared<url>(u, false);
		if(!url_->port && !url_->schema.compare(0, 5, "https")) {
			url_->port = 443;
		}else if(!url_->port && !url_->schema.compare(0, 4, "http")) {
			url_->port = 80;
		}
	}
	void client_request::build_ex(boost::beast::http::message<true, value_body<true>>& ctr_)
	{
		// if(url_) return; // 防止重复 build
		// 消息容器
		ctr_.method(boost::beast::http::string_to_verb(get("method").to_string()));
		std::string target(url_->path.length() > 0? url_->path : "/");
		if(!url_->query_raw.empty()) {
			target.append("?");
			target.append(url_->query_raw);
		}
		ctr_.target(target);
		// 头
		php::array header = get("header");
		for(auto i=header.begin(); i!=header.end(); ++i) {
			php::string key = i->first;
			php::string val = i->second;
			val.to_string();
			ctr_.set(boost::string_view(key.c_str(), key.size()), val);
		}
		if(ctr_.find(boost::beast::http::field::host) == ctr_.end()) {
			ctr_.set(boost::beast::http::field::host, url_->host);
		}
		// COOKIE
		php::array cookie = get("cookie");
		php::buffer cookies;
		for(auto i=cookie.begin(); i!=cookie.end(); ++i) {
			php::string key = i->first;
			php::string val = i->second.to_string();
			val = php::url_encode(val.c_str(), val.size());
			cookies.append(key);
			cookies.push_back('=');
			cookies.append(val);
			cookies.push_back(';');
			cookies.push_back(' ');
		}
		ctr_.set(boost::beast::http::field::cookie, php::string(std::move(cookies)));
		php::string body = get("body");
CTYPE_AGAIN:
		auto ctype = ctr_.find(boost::beast::http::field::content_type);
		if(ctype == ctr_.end()) {
			ctr_.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
			goto CTYPE_AGAIN;
		}
		if(body.empty()) return;
		ctr_.body() = ctype_encode(ctype->value(), body);
		ctr_.prepare_payload();
	}
}
}
