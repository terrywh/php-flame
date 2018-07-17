#include "../coroutine.h"
#include "http.h"
#include "value_body.h"
#include "handler.h"
#include "chunked_writer.h"
#include "file_writer.h"
#include "content_writer.h"
#include "server_request.h"
#include "server_response.h"
#include "../time/time.h"

namespace flame {
namespace http {
	void server_response::declare(php::extension_entry& ext) {
		php::class_entry<server_response> class_server_response("flame\\http\\server_response");
		class_server_response
			.property({"status", 200})
			.property({"header", nullptr})
			.property({"cookie", nullptr})
			.property({"body",   nullptr})
			.method<&server_response::__construct>("__construct", {}, php::PRIVATE)
			.method<&server_response::__destruct>("__destruct")
			.method<&server_response::set_cookie>("set_cookie", {
				{"name", php::TYPE::STRING},
				{"value", php::TYPE::STRING, false, true},
				{"expire", php::TYPE::INTEGER, false, true},
				{"path", php::TYPE::STRING, false, true},
				{"domain", php::TYPE::STRING, false, true},
				{"secure", php::TYPE::BOOLEAN, false, true},
				{"httponly", php::TYPE::BOOLEAN, false, true},
			})
			.method<&server_response::write_header>("write_header", {
				{"status", php::TYPE::INTEGER, false, true}
			})
			.method<&server_response::write>("write", {
				{"chunk", php::TYPE::UNDEFINED}
			})
			.method<&server_response::end>("end", {
				{"chunk", php::TYPE::UNDEFINED, false, true}
			})
			.method<&server_response::file>("file", {
				{"root", php::TYPE::STRING},
				{"path", php::TYPE::STRING},
			});

		ext.add(std::move(class_server_response));
	}
	// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
	php::value server_response::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value server_response::__destruct(php::parameters& params) {
		// Content-Length: xxxxx 用法使用对象销毁作为触发实际响应发送
		if(handler_ && !handler_->res_->chunked() && !(status_ & RESPONSE_STATUS_HEADER_SENT)) {
			// 头
			build_ex(*handler_->res_);
			// 体
			php::string body = get("body");
			if(!body.empty()) {
				body = ctype_encode(handler_->res_->find(boost::beast::http::field::content_type)->value(), body);
			}
			handler_->res_->body() = body;
			handler_->res_->prepare_payload();
			// 执行
			std::make_shared<content_writer>(nullptr, handler_, body, status_)->start();
			status_ |= RESPONSE_STATUS_HEADER_SENT | RESPONSE_STATUS_FINISHED;
		}
		return nullptr;
	}
	php::value server_response::set_cookie(php::parameters& params) {
		php::array cookie(4);
		php::string name = params[0], val;
		name.to_string();
		cookie.set("name", name);
		if(params.size() > 1) {
			val = params[1];
			val.to_string();
		}else{
			val = php::string("");
		}
		cookie.set("value", val);
		if(params.size() > 2) {
			// php::object expire = php::datetime(time::now() +  * 1000);
			cookie.set("expire", params[2].to_integer());
		}
		if(params.size() > 3) {
			php::string path = params[3];
			path.to_string();
			cookie.set("path", path);
		}
		if(params.size() > 4) {
			php::string domain = params[4];
			domain.to_string();
			cookie.set("domain", domain);
		}
		if(params.size() > 5) {
			bool secure = params[5];
			cookie.set("secure", secure);
		}
		if(params.size() > 6) {
			bool http_only = params[5];
			cookie.set("http_only", http_only);
		}

		php::array cookies = get("cookie", true);
		if(cookies.typeof(php::TYPE::NULLABLE)) {
			cookies = php::array(4);
		}
		cookies.set(cookies.size(), cookie);
		return nullptr;
	}
	// chunked encoding 用法
	php::value server_response::write_header(php::parameters& params) {
		if(status_ & RESPONSE_STATUS_HEADER_SENT) {
			throw php::exception(zend_ce_exception, "header already sent");
		}
		if(params.size() > 0) {
			set("status", params[0].to_integer());
		}
		handler_->res_->chunked(true);
		build_ex(*handler_->res_);
		std::make_shared<chunked_writer>(flame::coroutine::current, handler_, RESPONSE_TARGET_WRITE_HEADER, status_)
			->start();
		status_ |= RESPONSE_STATUS_HEADER_SENT;
		return coroutine::async();
	}
	php::value server_response::write(php::parameters& params) {
		if(status_ & RESPONSE_STATUS_FINISHED) {
			throw php::exception(zend_ce_exception, "response already done");
		}
		if(!(status_ & RESPONSE_STATUS_HEADER_SENT)) {
			handler_->res_->chunked(true);
			build_ex(*handler_->res_);
		}
		php::string chunk = params[0];
		chunk.to_string();
		std::make_shared<chunked_writer>(flame::coroutine::current, handler_, RESPONSE_TARGET_WRITE_CHUNK, chunk, status_)
			->start();
		status_ |= RESPONSE_STATUS_HEADER_SENT;
		return coroutine::async();
	}
	php::value server_response::end(php::parameters& params) {
		if(status_ & RESPONSE_STATUS_FINISHED) {
			throw php::exception(zend_ce_exception, "response already done");
		}
		if(!(status_ & RESPONSE_STATUS_HEADER_SENT)) {
			handler_->res_->chunked(true);
			build_ex(*handler_->res_);
		}
		if(params.size() > 0) {
			php::string chunk = params[0];
			chunk.to_string();
			std::make_shared<chunked_writer>(flame::coroutine::current, handler_, RESPONSE_TARGET_WRITE_CHUNK_LAST, chunk, status_)
				->start();
		}else{
			std::make_shared<chunked_writer>(flame::coroutine::current, handler_, RESPONSE_TARGET_WRITE_CHUNK_LAST, status_)
				->start();
		}
		status_ |= RESPONSE_STATUS_HEADER_SENT | RESPONSE_STATUS_FINISHED;
		return coroutine::async();
	}
	php::value server_response::file(php::parameters& params) {
		if(status_ & RESPONSE_STATUS_HEADER_SENT) {
			throw php::exception(zend_ce_exception, "header already sent");
		}
		if(status_ & RESPONSE_STATUS_FINISHED) {
			throw php::exception(zend_ce_exception, "response already done");
		}

		handler_->res_->chunked(true);
		build_ex(*handler_->res_);

		boost::filesystem::path root, file;
		root += params[0].to_string();
		file += params[1].to_string();
		std::make_shared<file_writer>(flame::coroutine::current, handler_, root / file.lexically_normal(), status_)->start();
		status_ |= RESPONSE_STATUS_HEADER_SENT | RESPONSE_STATUS_FINISHED;
		return coroutine::async();
	}
	server_response::server_response()
	: status_(0) {

	}
	// 支持 content-length 形式的用法
	void server_response::build_ex(boost::beast::http::message<false, value_body<false>>& ctr_) {
		if(status_ & RESPONSE_STATUS_HEADER_BUILT) return; // 重复使用
		status_ |= RESPONSE_STATUS_HEADER_BUILT; // 在 chunked_writer 中设置
		ctr_.result( get("status").to_integer() ); // 非法 status_code 会抛出异常
		php::array headers = get("header", true);
		if(headers.typeof(php::TYPE::ARRAY)) {
			for(auto i=headers.begin(); i!=headers.end(); ++i) {
				php::string key { i->first };
				i->second.to_string();

				ctr_.set(boost::string_view(key.c_str(), key.size()), i->second);
			}
		}
		ctr_.set(boost::beast::http::field::connection, ctr_.keep_alive() ? "keep-alive" : "close");
		php::array cookies = get("cookie", true);
		if(cookies.typeof(php::TYPE::ARRAY)) {
			for(auto i=cookies.begin(); i!=cookies.end(); ++i) {
				php::array  cookie = i->second;
				std::string buffer;
				buffer.append(cookie.get("name"));
				buffer.push_back('=');
				php::string value = cookie.get("value");
				if(!value.empty()) {
					buffer.push_back('"');
					buffer.append(php::url_encode(value.c_str(), value.size()));
					buffer.push_back('"');
				}
				if(cookie.exists("expire")) {
					std::int64_t expire = cookie.get("expire");
					// 范围 30 天内的数值, 按时间长度计算
					if(expire < 30 * 86400) {
						buffer.append("; Max-Age=", 10);
						buffer.append(std::to_string(expire));
						buffer.append("; Expire=", 9);
						php::object date = php::datetime(expire * 1000 + time::now());
						buffer.append(date.call("format", {"l, d-M-Y H:i:s T"}));
					}else{
					// 超过上面范围, 按时间点计算
						buffer.append("; Expire=", 9);
						php::object date = php::datetime(expire * 1000);
						buffer.append(date.call("format", {"l, d-M-Y H:i:s T"}));
					}
				}
				if(cookie.exists("path")) {
					buffer.append("; Path=", 7);
					buffer.append(cookie.get("path"));
				}
				if(cookie.exists("domain")) {
					buffer.append("; Domain=", 9);
					buffer.append(cookie.get("domain"));
				}
				if(!cookie.get("secure").empty()) {
					buffer.append("; Secure", 8);
				}
				if(!cookie.get("http_only").empty()) {
					buffer.append("; HttpOnly", 10);
				}
				ctr_.insert("Set-Cookie", buffer);
			}
		}
CTYPE_AGAIN:
		auto ctype = ctr_.find(boost::beast::http::field::content_type);
		if(ctype == ctr_.end()) {
			ctr_.set(boost::beast::http::field::content_type, "text/plain");
			goto CTYPE_AGAIN;
		}
	}
}
}
