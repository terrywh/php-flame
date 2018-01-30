#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../log/log.h"
#include "http.h"
#include "server_connection_base.h"
#include "server_connection.h"
#include "server_response_base.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace http {
// server_response::server_response() {

// }
server_response::~server_response() {
	conn_->close();
}
void server_response::buffer_header() {
	// STATUS_CODE STATUS_TEXT\r\n
	int          status_code = prop("status");
	std::string& status_text = flame::net::http::status_mapper[status_code];
	sprintf(
		buffer_.put(15 + status_text.length()),
		"HTTP/1.1 %03d %.*s\r\n",
		status_code, status_text.length(), status_text.c_str());

	// KEY: VALUE\r\n
	php::array &header = prop("header");
	if(header.has("Content-Length", 14)) {
		is_chunked = false;
		header.erase("Transfer-Encoding", 17);
	}
	for(auto i=header.begin(); i!=header.end(); ++i) {
		php::string& key = i->first;
		php::string  val = i->second.to_string();
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
		buffer_.adv(sprintf(buffer_.rev(16), "%x\r\n", size));
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
void server_response::init(server_connection_base* conn) {
	server_response_base::init(conn);
	php::array& header = prop("header",6);
	// 默认使用 chunked encoding 进行输出
	header.at("Transfer-Encoding", 17) = php::string("chunked",7);
	is_chunked = true;
}

}
}
}
