#include "client_response.h"

namespace flame {
namespace net {
namespace http {
	client_response::client_response() {
		std::memset(&header_parser_conf, 0, sizeof(kv_parser_settings));
		std::memset(&cookie_parser_conf, 0, sizeof(kv_parser_settings));

		header_parser_conf.on_key = header_key_cb;
		header_parser_conf.on_val = header_val_cb;
		header_parser_conf.w1 = '\r';
		header_parser_conf.w2 = '\n';
		header_parser_conf.s1 = ':';
		header_parser_conf.s2 = '\r';
		cookie_parser_conf.on_key = cookie_key_cb;
		cookie_parser_conf.on_val = cookie_val_cb;

		kv_parser_init(&header_parser);
		kv_parser_init(&cookie_parser);
		header_parser.data = this;
		cookie_parser.data = this;
	}
	void client_response::init() {

	}
	void client_response::head_cb(char* ptr, size_t size) {
		// 由于 curl 回调的 header 行完整，不需要考虑数据段问题
		// kv_parser_execute(&header_parser, &header_parser_conf, ptr, size);
	}
	void client_response::body_cb(char* ptr, size_t size) {
		std::memcpy(body_.put(size), ptr, size);
	}
	void client_response::done_cb(CURL* easy) {
		prop("body", 4)   = std::move(body_);
		prop("header", 6) = std::move(header_);
		prop("cookie", 6) = std::move(cookie_);
	}
	int client_response::header_key_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		// 由于 curl 回调的 header 行完整，不需要考虑数据段问题
		self->key_data = data;
		self->key_size = size;
		return size;
	}
	int client_response::header_val_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		std::printf("%.*s: %.*s", self->key_size, self->key_data, size, data);
		if(self->key_size == 10 && (std::strncmp(self->key_data, "Set-Cookie", 10) == 0
			|| std::strncmp(self->key_data, "set-cookie", 10) == 0)) {

			kv_parser_execute(&self->cookie_parser, &self->cookie_parser_conf, data, size);
		}else{
			self->header_.at(self->key_data, self->key_size) = php::string(data, size);
		}
		return size;
	}
	int client_response::cookie_key_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		std::printf("cookie key: [%.*s]\n", size, data);
		return size;
	}
	int client_response::cookie_val_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		std::printf("cookie val: [%.*s]\n", size, data);
		return size;
	}
}
}
}
