#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "write_buffer.h"
#include "response.h"
#include "request.h"

namespace net { namespace http {
	std::map<uint32_t, std::string> response::status_map {
		{100, "Continue"},
		{101, "Switching Protocols"},
		{102, "Processing"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{207, "Multi-Status"},
		{208, "Already Reported"},
		{226, "IM Used"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Payload Too Large"},
		{414, "URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{421, "Misdirected Request"},
		{422, "Unprocessable Entity"},
		{423, "Locked"},
		{424, "Failed Dependency"},
		{426, "Upgrade Required"},
		{428, "Precondition Required"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{451, "Unavailable For Legal Reasons"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"},
		{506, "Variant Also Negotiates"},
		{507, "Insufficient Storage"},
		{508, "Loop Detected"},
		{510, "Not Extended"},
		{511, "Network Authentication Required"},
	};

	void response::init(php::extension_entry& extension) {
		php::class_entry<response> net_http_response("flame\\net\\http\\response");
		net_http_response.add<&response::write_header>("write_header");
		net_http_response.add<&response::write>("write");
		net_http_response.add<&response::end>("end");
		net_http_response.add(php::property_entry("header", nullptr));
		net_http_response.add<response::build>("build");
		extension.add(std::move(net_http_response));
		return ;
	}

	php::value response::build(php::parameters& params) {
		php::object res_= php::object::create<response>();
		response*   res = res_.native<response>();
		php::object req_= params[0];
		if(!req_.is_instance_of<request>()) {
			throw php::exception("failed to build response: object of 'request' expected");
		}
		request* req = req_.native<request>();
		res->socket_ = req->socket_;
		res->header_["Content-Type"] = php::value("text/plain; charset=utf-8", 25);
		res->header_["Transfer-Encoding"] = php::value("chunked", 7);
		if(req->is_keep_alive()) {
			res->header_["Connection"] =  php::value("Keep-Alive", 10);
		}else{
			res->header_["Connection"] =  php::value("Close", 5);
		}
		res->prop("header") = res->header_;
		// 响应头部的 HTTP 版本
		res->buffer_.emplace_back(std::string("HTTP/1.1"));
		return std::move(res_);
	}

	php::value response::write_header(php::parameters& params) {
		build_header(params[0]);
		return php::value([this] (php::parameters& params) mutable -> php::value {
			php::callable done = params[0];
			boost::asio::async_write(
				*socket_, buffer_,
				[this, done] (boost::system::error_code err, std::size_t n) mutable {
					buffer_.clear();
					if(err) {
						done(core::error_to_exception(err));
					} else {
						done(nullptr, std::int64_t(n));
					}
				});
				return nullptr;
			});
	}

	void response::build_header(int status_code) {
		if(header_sent_) {
			throw php::exception("write header failed: header already sent");
		}
		header_sent_ = true;

		buffer_.emplace_back((boost::format(" %d %s\r\n")
			% status_code % response::status_map[status_code]).str());

		for(auto it = header_.begin(); it != header_.end(); ++it) {
			// 由于采用 chunked 编码，理论上不应该在处理 content-length 头部
			zend_string* data = it->second.to_string();
			buffer_.emplace_back((boost::format("%s: %s\r\n")
				% it->first.data() % data->val).str());
		}
		buffer_.emplace_back(std::string("\r\n",2));
	}


	php::value response::write(php::parameters& params) {
		if(ended_) {
			throw php::exception("write failed: reponse sent");
		}
		php::string body = params[0];
		if(!header_sent_) {
			build_header(200);
		}
		buffer_.emplace_back((boost::format("%x\r\n") % body.length()).str());
		buffer_.emplace_back(body.data(), body.length());
		buffer_.emplace_back(std::string("\r\n",2));
		return php::value([this, body] (php::parameters& params) mutable -> php::value {
			php::callable done = params[0];
			boost::asio::async_write(*socket_, buffer_,
				[this, done, body] (boost::system::error_code err, std::size_t n) mutable {
					buffer_.clear();
					if(err) {
						done(core::error_to_exception(err));
					} else {
						done(nullptr, true);
					}
				});
				return nullptr;
			});
	}

	php::value response::end(php::parameters& params) {
		if(ended_) {
			throw php::exception("write failed: response sent");
		}
		if(!header_sent_) {
			build_header(200);
		}
		ended_ = true;
		if(params.length() > 0) {
			php::string body = params[0];
			if(body.length() > 0) {
				buffer_.emplace_back((boost::format("%x\r\n") % body.length()).str());
				buffer_.emplace_back(write_buffer(body.data(), body.length()));
				buffer_.emplace_back(std::string("\r\n",2));
			}
		}
		buffer_.emplace_back(std::string("0\r\n\r\n",5));
		return php::value([this] (php::parameters& params) mutable -> php::value {
			php::callable done = params[0];
			boost::asio::async_write(*socket_, buffer_,
				[this, done] (boost::system::error_code err, std::size_t n) mutable {
					buffer_.clear();
					if(err) {
						done(core::error_to_exception(err));
					} else {
						done(nullptr, std::int64_t(n));
					}
				});
				return nullptr;
			});
	}
}}
