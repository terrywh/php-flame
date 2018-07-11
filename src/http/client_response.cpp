#include "http.h"
#include "value_body.h"
#include "client_response.h"

namespace flame {
namespace http {
	void client_response::declare(php::extension_entry& ext) {
		php::class_entry<client_response>  class_client_response("flame\\http\\client_response");
		class_client_response
			.property({"status", 3000})
			.property({"header", nullptr})
			.property({"body", nullptr})
			.property({"rawBody", ""})
			.method<&client_response::__construct>("__construct", {}, php::PRIVATE)
			.method<&client_response::to_string>("__toString");
		ext.add(std::move(class_client_response));
	}
	// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
	php::value client_response::__construct(php::parameters& params) {
		return nullptr;	
	}
	php::value client_response::to_string(php::parameters& params) {
		return get("rawBody");
	}
	void client_response::build_ex() {
		set("status", ctr_.result_int());
		php::array header = get("header", true);
		if(header.typeof(php::TYPE::NULLABLE)) {
			header = php::array(4);
		}
		for(auto i=ctr_.begin(); i!=ctr_.end(); ++i) {
			php::string key {i->name_string().data(), i->name_string().size()};
			php::string val {i->value().data(), i->value().size()};

			// TODO 多个同名 HEADER 的处理
			php::lowercase_inplace(key.data(), key.size());
			header.set(key, val);
		}
		php::string body = ctr_.body();
		if(body.typeof(php::TYPE::STRING)) {
			set("rawBody", body);
			auto ctype = ctr_.find(boost::beast::http::field::content_type);
			if(ctype == ctr_.end()) {
				set("body", body);
			}else{
				set("body", ctype_decode(ctype->value(), body));
			}	
		}
	}
}
}
