#include "../coroutine.h"
#include "http.h"
#include "value_body.h"
#include "server_request.h"

namespace flame {
namespace http {
	void server_request::declare(php::extension_entry& ext) {
		php::class_entry<server_request> class_server_request(
			"flame\\http\\server_request");
		class_server_request
			.property({"method", "GET"})
			.property({"path", "/"})
			.property({"query", nullptr})
			.property({"header", nullptr})
			.property({"cookie", nullptr})
			.property({"body", nullptr})
			.property({"raw_body", ""})
			.property({"file", nullptr})
			.property({"data", nullptr})
			.method<&server_request::__construct>("__construct", {}, php::PRIVATE);

		ext.add(std::move(class_server_request));
	}
	server_request::server_request() {
		
	}
	server_request::~server_request() {

	}
	php::value server_request::__construct(php::parameters& params) {
		return nullptr;
	}
	void server_request::build_ex(const boost::beast::http::message<true, value_body<true>>& ctr_) {
		auto method = boost::beast::http::to_string(ctr_.method());
		set("method", php::string(method.data(), method.size()));
		// 目标请求地址
		auto target = ctr_.target();
		std::shared_ptr<php::url> url_ = php::parse_url(target.data(), target.size());
		if (url_->path) {
			set("path", php::string(url_->path));
		}
		else {
			set("path", php::string("/", 1));
		}
		if (url_->query) {
			php::array query(4);
			php::callable("parse_str")({php::string(url_->query), query.make_ref()});
			set("query", query);
		}
		php::array header(4);
		for (auto i=ctr_.begin(); i!=ctr_.end(); ++i) {
			php::string key {i->name_string().data(), i->name_string().size()};
			php::string val {i->value().data(), i->value().size()};

			if (key.size() == 6 && strncasecmp(key.c_str(), "cookie", 6) == 0) {
				php::array cookie(4);
				parser::separator_parser<std::string, php::buffer> p1('\0','\0','=','\0','\0',';', [&cookie] (std::pair<std::string, php::buffer> entry) {
					if (entry.second.size() > 0) {
						entry.second.shrink( php::url_decode_inplace(entry.second.data(), entry.second.size()) );
						cookie.set(entry.first, php::string(std::move(entry.second)));
					}
					else {
						cookie.set(entry.first, php::string(0));
					}
				});
				p1.parse(val.c_str(), val.size());
				p1.parse(";", 1); // 结尾分隔符可能不存在(可以认为数据行可能不完整)
				set("cookie", cookie);
			}
			// TODO 多个同名 HEADER 的处理
			php::lowercase_inplace(key.data(), key.size());
			header.set(key, val);
		}
		set("header", header);

		php::string body = ctr_.body();
		if (body.typeof(php::TYPE::STRING)) {
			auto ctype = ctr_.find(boost::beast::http::field::content_type);
			if (ctype == ctr_.end() || ctype->value().compare(0, 19, "multipart/form-data") != 0) {
				set("raw_body", body); // 不在 multipart 时保留原始数据
			}
			if (ctype != ctr_.end()) { // 存在时按照类型进行解析
				php::array files(4);
				set("body", ctype_decode(ctype->value(), body, &files));
				set("file", files);
			}
			else { // 不存在与 raw_body 相同
				set("body", body);
			}
		}
	}
}
}
