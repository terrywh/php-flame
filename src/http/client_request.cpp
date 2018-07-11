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
			.property({"body", ""})
			.method<&client_request::__construct>("__construct", {
				{"url", php::TYPE::STRING},
				{"body", php::TYPE::UNDEFINED, false, true},
				{"timeout", php::TYPE::INTEGER, false, true},
			})
			.method<&client_request::to_string>("__toString");

		ext.add(std::move(class_client_request));
	}
	php::value client_request::__construct(php::parameters& params) {
		if (params.length() > 2) {
			set("timeout", params[2]);
		}else{
			set("timeout", 3000);
		}
		if (params.length() > 1) {
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
	php::value client_request::to_string(php::parameters& params) {
		std::stringstream os;
		build_ex();
		os << ctr_;
		return os.str();
	}
	void client_request::build_ex() {
		if(url_) return; // 防止重复 build
		// 目标请求地址
		url_ = php::parse_url(get("url", true));
		 if(!url_->port && strncasecmp(url_->scheme, "https", 5) == 0) {
			url_->port = 443;
		}else if(!url_->port && strncasecmp(url_->scheme, "http", 4) == 0) {
			url_->port = 80;
		}
		// 消息容器
		ctr_.method(boost::beast::http::string_to_verb(get("method").to_string()));
		std::string target(url_->path ? url_->path : "/");
		if(url_->query) {
			target.append("?");
			target.append(url_->query);
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
