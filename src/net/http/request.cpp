#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "request.h"
#include "request_header_parser.h"

namespace net { namespace http {

	void request::init(php::extension_entry& extension) {
		php::class_entry<request> net_http_request("flame\\net\\http\\request");
		net_http_request.add(php::property_entry("version", "HTTP/1.1"));
		net_http_request.add(php::property_entry("method", "GET"));
		net_http_request.add(php::property_entry("path", "/"));
		net_http_request.add(php::property_entry("get", nullptr));
		net_http_request.add(php::property_entry("header", nullptr));
		net_http_request.add(php::property_entry("cookie", nullptr));
		net_http_request.add(php::property_entry("post", nullptr));
		net_http_request.add<request::parse>("parse");
		net_http_request.add<&request::body>("body");
		extension.add(std::move(net_http_request));
		return ;
	}

	php::value request::parse(php::parameters& params) {
		php::object tcp_socket = params[0];
		if(!tcp_socket.is_instance_of<net::tcp_socket>()) {
			throw php::exception("failed to parse request: object of flame\\net\\tcp_socket expected");
		}

		php::object req = php::object::create<request>();
		request* self = req.native<request>();
		self->socket_ = &tcp_socket.native<net::tcp_socket>()->socket_;
		// 异步流程
		return php::value([self, req](php::parameters& params) mutable -> php::value {
			php::callable done = params[0];
			self->read_head(req, done);
			return nullptr;
		});
	}

	void request::read_head(php::object& req, php::callable& done) {
		boost::asio::async_read_until(*socket_, buffer_, "\r\n\r\n",
			[this, req, done] (const boost::system::error_code err, std::size_t n) mutable {
				if(err == boost::asio::error::eof) {
					// 未接到请求，客户端关闭了连接
					done(nullptr, nullptr);
				} else if(err) {
					done(core::error_to_exception(err));
				} else {
					request_header_parser parser(this);
					if(!parser.parse(buffer_, n) || !parser.complete()) {
						done(core::error("failed to read request: illegal request"));
						return;
					}
					read_body(req, done);
				}
			});
	}

	void request::read_body(php::object& req, php::callable& done) {
		// 不支持 chunked 形式的请求
		// 简化的 content-length 逻辑（存在 content-length 存在 body 否则无 body）
		php::value* length_= header_.find("content-length");
		if(length_ == nullptr) {
			done(nullptr, req);
			return;
		}
		std::size_t length = header_["content-length"].to_long();
		if(length <= 0) {
			done(nullptr, req);
			return;
		}

		if(buffer_.size() < length) { // 现在 buffer_ 中还没有完整接收 body
			boost::asio::async_read(*socket_, buffer_,
				boost::asio::transfer_exactly(length - buffer_.size()),
				[this, req, done] (const boost::system::error_code err, std::size_t n) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						parse_body();
						done(nullptr, req);
					}
				});
		}else if(buffer_.size() == length) {
			parse_body();
			done(nullptr, req);
		}else{
			done(std::string("failed to read body: illegal request"), 0);
		}
	}

	void request::parse_body() {
		zend_string* type_ = header_.at("content-type", 12);
		php::strtolower(type_->val, type_->len);
		// 下述两种 content-type 自动解析：
		// 1. application/x-www-form-urlencoded 33
		// 2. application/json 16
		if(type_->len >=33 && std::memcmp(type_->val, "application/x-www-form-urlencoded", 33) == 0) {
			//zend_string* str = body.c_str();
			prop("post") = php::parse_str('&', buffer_, buffer_.size());
		}
		// 其他 content-type 原始 body 内容调用 body() 方法获取
	}

	php::value request::body(php::parameters& params) {
		php::string r(buffer_.size());
		// !!! 这里的 body 数据是 “复制” 的，代价还是比较大的；
		// TODO 考虑提供流方式的 body 获取方式？
		buffer_.sgetn(r.data(), buffer_.size());
		return std::move(r);
	}

	bool request::is_keep_alive() {
		php::value* c = header_.find("connection");
		if(c == nullptr) return false;
		php::string s = *c;
		php::strtolower(s.data(), s.length());
		if(std::strncmp(s.data(), "keep-alive", std::min(std::size_t(10), s.length())) == 0) {
			return true;
		}
		return false;
	}
}}
