#include "../coroutine.h"
#include "value_body.h"
#include "client_request.h"
#include "client_body.h"
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
			})
			.method<&client_request::__destruct>("__destruct");

		ext.add(std::move(class_client_request));
	}

	php::value client_request::__construct(php::parameters& params) {
		if (params.length() > 2) set("timeout", params[2]);
		else set("timeout", 3000);

		if (params.length() > 1 && !params[1].empty()) {
			set("body", params[1]);
			set("method", "POST");
		}else{
			set("method", "GET");
		}
		set("url", params[0]);
		set("header", php::array(0));
		set("cookie", php::array(0));

		c_easy_ = curl_easy_init();
		return nullptr;
	}

	php::value client_request::__destruct(php::parameters &params) {
		if(c_head_) curl_slist_free_all(c_head_);
		if(c_easy_) curl_easy_cleanup(c_easy_);
		return nullptr;
	}

	void client_request::build_ex() {
		// 目标请求地址
		// ---------------------------------------------------------------------------
        php::string u = get("url", true);
		if(!u.typeof(php::TYPE::STRING)) {
			throw php::exception(zend_ce_exception, "request 'url' must be a string");
		}
		curl_easy_setopt(c_easy_, CURLOPT_URL, u.c_str());
		php::string m = get("method", true);
		curl_easy_setopt(c_easy_, CURLOPT_CUSTOMREQUEST, m.c_str());
		// 头
		// ---------------------------------------------------------------------------
		if(c_head_ != nullptr) curl_slist_free_all(c_head_);
		std::string ctype;
		php::array header = get("header");
		for(auto i=header.begin(); i!=header.end(); ++i) {
			std::string key = i->first;
			std::string val = i->second;
			if(strncasecmp(key.c_str(), "content-type", 12) == 0) {
				ctype = val;
			}
			c_head_ = curl_slist_append(c_head_, (boost::format("%s: %s") % key % val).str().c_str());
		}
		curl_easy_setopt(c_easy_, CURLOPT_HTTPHEADER, c_head_);
		// COOKIE
		// ---------------------------------------------------------------------------
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
		php::string cookie_str = std::move(cookies);
		curl_easy_setopt(c_easy_, CURLOPT_COOKIE, cookie_str.c_str());
		// 体
		// ---------------------------------------------------------------------------
		php::string body = get("body");
		if(ctype.empty()) ctype.assign("application/x-www-form-urlencoded", 33);
		if(body.empty()) {

		}else if(body.instanceof(php::class_entry<client_body>::entry())) {
			// TODO multipart support
		}else{
			body = ctype_encode(ctype, body);
			// 注意: CURLOPT_POSTFIELDS 仅"引用" body 数据
			set("body", body);
			curl_easy_setopt(c_easy_, CURLOPT_POSTFIELDS, body.c_str());
		}
	}
}
}
