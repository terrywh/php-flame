#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../log/log.h"
#include "http.h"
#include "server_connection_base.h"
#include "server_connection.h"
#include "server_response_base.h"

namespace flame {
namespace net {
namespace http {

php::value server_response_base::set_cookie(php::parameters& params) {
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
php::value server_response_base::write_header(php::parameters& params) {
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
php::value server_response_base::write(php::parameters& params) {
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
php::value server_response_base::end(php::parameters& params) {
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
	buffer_ending();

	return write_buffer(params);
}
php::value server_response_base::write_buffer(php::parameters& params) {
	if(conn_->write(std::move(buffer_)
		, coroutine::current
		, params.length() > 0 && params[params.length()-1].is_true())
	) {
		return flame::async(this);
	}
	return php::BOOL_NO;
}
void server_response_base::init(server_connection_base* conn) {
	conn_ = conn;
	conn_->refer();
	php::array header(1);
	header.at("Content-Type") = php::string("text/plain", 10);
	prop("header",6) = std::move(header);
}
void server_response_base::buffer_ending() {
	// 默认状态无填充结束数据
}

}
}
}
