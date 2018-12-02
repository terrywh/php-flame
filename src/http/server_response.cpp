#include "../coroutine.h"
#include "../time/time.h"
#include "_handler.h"
#include "server_response.h"

namespace flame::http {
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
	server_response::server_response()
	: status_(0) {

	}
	server_response::~server_response()
	{

	}
	// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
	php::value server_response::__construct(php::parameters& params)
	{
		return nullptr;
	}
	php::value server_response::__destruct(php::parameters& params)
	{
		// 响应还未完全结束, 由 handler 代理 last_chunk 响应
		if(handler_) 
		{
			coroutine_handler ch{coroutine::current};
			handler_->finish(this, ch);
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
		if((status_ & STATUS_HEAD_SENT) || (status_ & STATUS_BODY_SENT)) {
			throw php::exception(zend_ce_exception, "header already sent");
		}
		status_ |= STATUS_HEAD_SENT;

		if(params.size() > 0) {
			set("status", params[0].to_integer());
		}
		coroutine_handler ch{coroutine::current};
		handler_->write_head(this, ch);
		return nullptr;
	}
	php::value server_response::write(php::parameters& params) {
		if(status_ & STATUS_BODY_END)
		{
			throw php::exception(zend_ce_exception, "response already done");
		}
		coroutine_handler ch{coroutine::current};
		if (!(status_ & STATUS_HEAD_SENT))
		{
			status_ |= STATUS_HEAD_SENT;
			handler_->write_head(this, ch);
		}
		status_ |= STATUS_BODY_SENT;
		php::string chunk = params[0].to_string();
		handler_->write_body(this, chunk, ch);
		return nullptr;
	}
	php::value server_response::end(php::parameters& params) {
		if (status_ & STATUS_BODY_END)
		{
			throw php::exception(zend_ce_exception, "response already done");
		}
		coroutine_handler ch{coroutine::current};
		if (!(status_ & STATUS_HEAD_SENT))
		{
			status_ |= STATUS_HEAD_SENT;
			handler_->write_head(this, ch);
		}
		if(params.size() > 0) {
			status_ |= STATUS_BODY_SENT;
			php::string chunk = params[0].to_string();
			handler_->write_body(this, chunk, ch);
		}else{
			status_ |= STATUS_BODY_END;
			handler_->write_end(this, ch);
			handler_.reset();
		}
		return nullptr;
	}
	php::value server_response::file(php::parameters& params) {
		if ((status_ & STATUS_BODY_END) || (status_ & STATUS_BODY_SENT))
		{
			throw php::exception(zend_ce_exception, "response already done");
		}
		coroutine_handler ch{coroutine::current};
		if (!(status_ & STATUS_HEAD_SENT))
		{
			status_ |= STATUS_HEAD_SENT;
			handler_->write_head(this, ch);
		}
		status_ |= STATUS_BODY_END | STATUS_BODY_SENT;

		boost::filesystem::path root, file;
		root += params[0].to_string();
		file += params[1].to_string();

		handler_->write_file(this, (root / file.lexically_normal()).string(), ch);
		return nullptr;
	}
	
	// 支持 content-length 形式的用法
	void server_response::build_ex(boost::beast::http::message<false, value_body<false>>& ctr_) {
		if(status_ & STATUS_BUILT) return; // 重复使用
		status_ |= STATUS_BUILT; // 在 chunked_writer 中设置
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
						php::object date = php::datetime(expire * 1000 + std::chrono::duration_cast<std::chrono::milliseconds>(time::now().time_since_epoch()).count());
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
