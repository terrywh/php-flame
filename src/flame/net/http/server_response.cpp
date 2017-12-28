#include "../../coroutine.h"
#include "../../log/log.h"
#include "http.h"
#include "server_connection_base.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace http {
void server_response::init(server_connection* conn) {
	conn_ = conn;
	conn_->refer();
	
	// 默认使用 chunked encoding 进行输出
	php::array header(1);
	header.at("Transfer-Encoding", 17) = php::string("chunked",7);
	prop("header",6) = header;
}
server_response::~server_response() {
	// HTTP 协议标志：连接保持
	if(!http_should_keep_alive(&conn_->hpp_)) {
		conn_->close();
	}
}
php::value server_response::set_cookie(php::parameters& params) {
	if(is_head_sent) {
		throw php::exception("header already sent");
	}
	php::buffer data;
	
	std::string name = params[0];
	std::memcpy(data.put(name.length()), name.c_str(), name.length());
	if(params.length() > 1) {
		php::string val = params[1].to_string();
		val = php::url_encode(val.c_str(), val.length());
		data.add('=');
		std::memcpy(data.put(val.length()), val.c_str(), val.length());
	}else{
		std::memcpy(data.put(58), "=deleted; Expires=Thu, 01 Jan 1970 00:00:00 GMT", 58);
	}
	if(params.length() > 2) {
		int64_t ts = params[0].to_long();
		if(ts > 0) {
			struct tm* expire = gmtime((time_t*)&ts);
			strftime(data.put(39), 39, "; Expires=%a, %d %b %Y %H:%M:%S GMT", expire);
		}
	}
	if(params.length() > 3 && params[3].is_string()) {
		php::string& path = params[3];
		sprintf(data.put(7+path.length()), "; Path=%.*s", path.length(), path.c_str());
	}
	if(params.length() > 4 && params[4].is_string()) {
		php::string& domain = params[4];
		sprintf(data.put(9+domain.length()), "; Domain=%.*s", domain.length(), domain.c_str());
	}
	if(params.length() > 5) {
		if(params[5].to_bool()) {
			std::memcpy(data.put(8), "; Secure", 8);
		}
	}
	if(params.length() > 6) {
		if(params[6].to_bool()) {
			std::memcpy(data.put(10), "; HttpOnly", 10);
		}
	}
	cookie_[name] = std::move(data);
	return nullptr;
}
php::value server_response::write_header(php::parameters& params) {
	if(is_head_sent || is_body_sent) {
		throw php::exception("header already sent");
	}
	if(params.length() > 0) {
		prop("status") = static_cast<int>(params[0]);
	}
	buffer_header();
	is_head_sent = true;
	
	return write_buffer(params);
}
php::value server_response::write(php::parameters& params) {
	if(is_body_sent) {
		throw php::exception("body already ended");
	}
	if(!is_head_sent) {
		buffer_header();
		is_head_sent = true;
	}
	php::string data = params[0].to_string();
	buffer_body(data.data(), data.length());

	return write_buffer(params);
}
php::value server_response::end(php::parameters& params) {
	if(is_body_sent) {
		throw php::exception("body already ended");
	}
	is_body_sent = true;
	if(!is_head_sent) {
		buffer_header();
		is_head_sent = true;
	}
	if(params.length() > 0) {
		php::string data = params[0].to_string();
		buffer_body(data.data(), data.length());
	}
	buffer_body(nullptr, 0);

	return write_buffer(params);
}
void server_response::buffer_header() {
	// STATUS_CODE STATUS_TEXT\r\n
	int          status_code = prop("status");
	std::string& status_text = flame::net::http::status_mapper[status_code];
	sprintf(
		buffer_.put(14 + status_text.length()),
		"HTTP/1.1 %03d %.*s\r\n",
		status_code, status_text.length(), status_text.c_str());

	// KEY: VALUE\r\n
	php::array &header = prop("header");
	for(auto i=header.begin(); i!=header.end(); ++i) {
		php::string& key = i->first;
		php::string  val = i->second.to_string();
		if(key.length() == 17
			&& strncasecmp(key.c_str(), "Transfer-Encoding", 17) == 0
			&& val.length() == 7
			&& strncasecmp(val.c_str(), "chunked", 7) == 0) {
			
			is_chunked = true;
		}
		sprintf(buffer_.put(key.length() + val.length() + 4),
			"%.*s: %.*s\r\n", key.length(), key.data(),
			val.length(), val.data());
	}
	// Set-Cookie: .....
	for(auto i=cookie_.begin(); i!=cookie_.end(); ++i) {
		sprintf(buffer_.put(14 + i->second.length()),
			"Set-Cookie: %.*s\r\n", i->second.length(), i->second.c_str());
	}
	buffer_.add('\r');
	buffer_.add('\n');
}
void server_response::buffer_body(const char* data, unsigned short size) {
	if(is_chunked) { // 默认状态（参见 server_connection 中创建过程）
		// * length
		fmt::ArrayWriter aw(buffer_.rev(16), 16);
		aw.write("{0:x}\r\n", size);
		buffer_.adv(aw.size());
		// * data
		if(size > 0 && data != nullptr) {
			std::memcpy(buffer_.put(size), data, size);
		}
		buffer_.add('\r');
		buffer_.add('\n');
	}else if(size > 0) { // content-length or something unknown
		std::memcpy(buffer_.put(size), data, size);
	}
}
php::value server_response::write_buffer(php::parameters& params) {
	if(conn_->write(std::move(buffer_), coroutine::current)) return flame::async(this);
	if(params.length() > 0 && !params[params.length()-1].is_true()) {
		log::default_logger->write("(WARN) write failed: connection already closed");
	}
	return php::BOOL_NO;
}

}
}
}
