#include "../../fiber.h"
#include "server_response.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "../http/http.h"

namespace flame {
namespace net {
namespace fastcgi {

server_response::server_response() {
	php::array header(2);
	header["Content-Type"] = php::string("text/plain", 10);
	prop("header") = header;
}
server_response::~server_response() {
	// 强制的请求结束
	if((conn_->fpp_.flag & FASTCGI_FLAGS_KEEP_CONN) == 0 && !prop("ended").is_true()) {
		conn_->close();
	}
}

#define CACULATE_PADDING(size) (size) % 8 == 0 ? 0 : 8 - (size) % 8;

php::value server_response::write_header(php::parameters& params) {
	if(prop("header_sent").is_true() || prop("ended").is_true()) {
		php::warn("header already sent");
		return nullptr;
	}
	if(params.length() >= 1) {
		prop("status") = static_cast<int>(params[0]);
	}
	buffer_head();
	prop("header_sent") = true;
	buffer_write();
	return flame::async();
}

void server_response::buffer_head() {
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
	php::array header = prop("header");
	for(auto i=header.begin(); i!=header.end(); ++i) {
		php::string& key = i->first;
		php::string& val = i->second.to_string();
		sprintf(
			buffer_.put(key.length() + val.length() + 4),
			"%.*s: %.*s\r\n",
			key.length(), key.data(), val.length(), val.data());
	}
	sprintf(buffer_.put(2), "\r\n");
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_STDOUT;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = conn_->fpp_.request_id;
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

php::value server_response::write(php::parameters& params) {
	if(prop("ended").is_true()) {
		php::warn("response already ended");
		return nullptr;
	}
	if(!prop("header_sent").is_true()) {
		buffer_head();
		prop("header_sent") = true;
	}
	// TODO 若实际传递的 data 大于可容纳的 body 最大值 64k，需要截断若干次发送 buffer_body 发送
	php::string& data = params[0].to_string();
	buffer_body(data.data(), data.length());
	buffer_write();
	return flame::async();
}

void server_response::buffer_body(const char* data, unsigned short size) {
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_STDOUT;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = conn_->fpp_.request_id;
	// 字节序调整
	header_.content_length = (size & 0x00ff) << 8 | (size & 0xff00) >> 8;
	header_.padding_length = CACULATE_PADDING(size);
	header_.reserved       = 0;
	// 头部
	std::memcpy(buffer_.put(sizeof(header_)), &header_, sizeof(header_));
	// 内容
	std::memcpy(buffer_.put(size), data, size);
	// 填充
	if(header_.padding_length > 0) {
		std::memset(buffer_.put(header_.padding_length), 0, header_.padding_length);
	}
}

php::value server_response::end(php::parameters& params) {
	if(prop("ended").is_true()) {
		php::warn("response already ended");
		return nullptr;
	}
	prop("ended") = true;
	if(!prop("header_sent").is_true()) {
		buffer_head();
		prop("header_sent") = true;
	}
	if(params.length() >= 1) {
		// TODO 若实际传递的 data 大于可容纳的 body 最大值 64k，需要截断若干次发送 buffer_body 发送
		php::string& data = params[0].to_string();
		buffer_body(data.data(), data.length());
	}
	buffer_ending();
	buffer_write();
	return flame::async();
}

void server_response::buffer_ending() {
	// 根据长度填充头部
	header_.version        = FASTCGI_VERSION;
	header_.type           = FASTCGI_TYPE_STDOUT;
	// !!! 解析过程没有反转，这里也不需要
	header_.request_id     = conn_->fpp_.request_id;
	header_.content_length = 0;
	header_.padding_length = 0;
	header_.reserved       = 0;
	// 头部
	std::memcpy(buffer_.put(sizeof(header_)), &header_, sizeof(header_));
	// 无内容 无填充
	header_.type = FASTCGI_TYPE_END_REQUEST;
	// length = 8 字节序调整
	header_.content_length = 0x0800;
	// 头部
	std::memcpy(buffer_.put(sizeof(header_)), &header_, sizeof(header_));
	// 内容
	std::memset(buffer_.put(8), 0, 8);
	// 无填充
}

void server_response::buffer_write() {
	uv_write_t* req = new uv_write_t;
	req->data = flame::this_fiber()->push(this);
	uv_buf_t    buf {buffer_.data(), (size_t)buffer_.size()};
	int error = uv_write(req, reinterpret_cast<uv_stream_t*>(&conn_->socket_), &buf, 1, write_cb);
	if(0 > error) {
		flame::this_fiber()->ignore_warning(uv_strerror(error), error);
	}
}

void server_response::write_cb(uv_write_t* req, int status) {
	flame::fiber*   fiber = reinterpret_cast<flame::fiber*>(req->data);
	server_response* self = fiber->context<server_response>();
	delete req;

	int size = self->buffer_.size();
	self->buffer_.reset();
	// 若 Web 服务器没有保持连接的标记，在请求结束后关闭连接
	if((self->conn_->fpp_.flag & FASTCGI_FLAGS_KEEP_CONN) == 0 && self->prop("ended").is_true()) {
		self->conn_->close();
	}
	if(status < 0) {
		php::info("write/end failed: %s", uv_strerror(status));
		fiber->next(nullptr);
	}else{
		fiber->next(size);
	}
}

}
}
}
