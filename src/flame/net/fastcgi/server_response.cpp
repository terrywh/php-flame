#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../log/log.h"
#include "../http/http.h"
#include "../http/server_connection_base.h"
#include "../http/server_response_base.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {

#define CACULATE_PADDING(size) (size) % 8 == 0 ? 0 : 8 - (size) % 8;
server_response::server_response() {
	
}
server_response::~server_response() {
	conn_->close();
}
void server_response::buffer_header() {
	// 预留头部
	char* head = buffer_.put(sizeof(header_));
	// STATUS_CODE STATUS_TEXT\r\n
	int          status_code = prop("status");
	std::string& status_text = flame::net::http::status_mapper[status_code];
	sprintf(
		buffer_.put(14 + status_text.length()),
		"Status: %03d %.*s\r\n", // fastcgi 返回方式与正常 HTTP 情况不同
		status_code, status_text.length(), status_text.c_str());
	// KEY: VALUE\r\n
	php::array &header = prop("header");
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
	
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_STDOUT;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = dynamic_cast<server_connection*>(conn_)->fpp_.request_id;
	unsigned short length  = buffer_.size() - sizeof(header_);
	// 注意字节序调整
	header_.content_length = (length & 0x00ff) << 8 | (length & 0xff00) >> 8;
	header_.padding_length = CACULATE_PADDING(length);
	header_.reserved       = 0;
	std::memcpy(head, &header_, sizeof(header_));
	// padding
	if(header_.padding_length > 0) {
		std::memset(buffer_.put(header_.padding_length), 0, header_.padding_length);
	}
}
void server_response::buffer_body(const char* data, unsigned short size) {
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_STDOUT;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = dynamic_cast<server_connection*>(conn_)->fpp_.request_id;
	// 字节序调整
	header_.content_length = (size & 0x00ff) << 8 | (size & 0xff00) >> 8;
	header_.padding_length = CACULATE_PADDING(size);
	header_.reserved       = 0;
	// 头部
	std::memcpy(buffer_.put(sizeof(header_)), &header_, sizeof(header_));
	// 内容
	if(size > 0) {
		std::memcpy(buffer_.put(size), data, size);
	}
	// 填充
	if(header_.padding_length > 0) {
		std::memset(buffer_.put(header_.padding_length), 0, header_.padding_length);
	}
}
void server_response::buffer_ending() {
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_END_REQUEST;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = dynamic_cast<server_connection*>(conn_)->fpp_.request_id;
	header_.content_length = 0x0800; // length = 8 字节序调整
	header_.padding_length = 0;
	header_.reserved       = 0;
	// 头部
	std::memcpy(buffer_.put(sizeof(header_)), &header_, sizeof(header_));
	// 内容
	std::memset(buffer_.put(8), 0, 8);
	// 无填充
}

}
}
}
