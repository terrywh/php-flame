#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "client_response.h"

namespace flame {
namespace net {
namespace http {
	client_response::client_response()
	: header_(0)
	, cookie_(0) {
		std::memset(&header_parser_conf, 0, sizeof(kv_parser_settings));
		std::memset(&cookie_parser_conf, 0, sizeof(kv_parser_settings));

		header_parser_conf.on_key = header_key_cb;
		header_parser_conf.on_val = header_val_cb;
		header_parser_conf.s1 = ':';
		header_parser_conf.s2 = '\r';
		header_parser_conf.w1 = '\n';
		header_parser_conf.w2 = '\0';

		cookie_parser_conf.on_key = cookie_key_cb;
		cookie_parser_conf.on_val = cookie_val_cb;
		cookie_parser_conf.s1 = '=';
		cookie_parser_conf.s2 = ';';
		cookie_parser_conf.w1 = '\n';
		cookie_parser_conf.w2 = '\r';
		kv_parser_init(&header_parser);
		kv_parser_init(&cookie_parser);

		header_parser.data = this;
		cookie_parser.data = this;
	}
	void client_response::head_cb(char* ptr, size_t size) {
		// 由于 curl 回调的 header 行完整，不需要考虑数据段问题
		kv_parser_execute(&header_parser, &header_parser_conf, ptr, size);
	}
	void client_response::body_cb(char* ptr, size_t size) {
		std::memcpy(body_.put(size), ptr, size);
	}
	void client_response::done_cb(long status) {
		prop("status", 6) = status;
		prop("header", 6) = std::move(header_);
		prop("cookie", 6) = std::move(cookie_);
		prop("body", 4)   = std::move(body_);
	}
	int client_response::header_key_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		// 由于 curl 回调的 header 行完整，不需要考虑数据段问题
		php::strtolower_inplace(const_cast<char*>(data), size); // 统一响应头 KEY 大小写
		self->key_data = data;
		self->key_size = size;
		return 0;
	}
	int client_response::header_val_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		if(self->key_size == 10 && std::strncmp(self->key_data, "set-cookie", 10) == 0) {
			self->cookie_item = nullptr;
			kv_parser_reset(&self->cookie_parser);
			kv_parser_execute(&self->cookie_parser, &self->cookie_parser_conf, data, size);
			self->cookie_item = nullptr;
		}else if(self->key_size > 0) {
			self->header_.at(self->key_data, self->key_size) = php::string(data, size);
		}
		self->key_size = 0;
		return 0;
	}
	int client_response::cookie_key_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		self->key_data = data;
		self->key_size = size;
		return 0;
	}
	int client_response::cookie_val_cb(kv_parser* parser, const char* data, size_t size) {
		client_response* self = reinterpret_cast<client_response*>(parser->data);
		if(self->key_size > 0) {
			if(self->cookie_item.is_null()) {
				self->cookie_item = php::array(0);
				self->cookie_.at(self->key_data, self->key_size) = self->cookie_item;
			}else{
				self->cookie_item.at(self->key_data, self->key_size) = php::string(data, size);
			}
		}
		self->key_size = 0;
		return 0;
	}
}
}
}
