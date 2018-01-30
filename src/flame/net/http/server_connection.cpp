#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../http/server_connection_base.h"
#include "../http/server_response_base.h"
#include "../http/server_request.h"
#include "http.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace http {
	server_connection::server_connection(void* data)
	: http::server_connection_base(data)
	, query_(nullptr) 
	, header_(nullptr) 
	, cookie_(nullptr) {
		http_parser_settings_init(&hps_);
		// !!! HTTP 解析过程中，数据并不一定完整，需要缓存 !!!
		hps_.on_url              = hp_url_cb;
		hps_.on_header_field     = hp_header_field_cb;
		hps_.on_header_value     = hp_header_value_cb;
		hps_.on_headers_complete = hp_header_complete_cb;
		hps_.on_body             = hp_body_cb;
		hps_.on_message_complete = hp_message_complete;
		http_parser_init(&hpp_, HTTP_REQUEST);
		hpp_.data = this;
	}
	server_connection::~server_connection() {
		
	}
	ssize_t server_connection::parse(const char* data, ssize_t size) {
		return http_parser_execute(&hpp_, &hps_, data, size);
	}
	void server_connection::close() {
		if(--refer_ == 0 && !http_should_keep_alive(&hpp_)) {
			server_connection_base::close_ex();
		}
	}
	int server_connection::hp_url_cb(http_parser* parser, const char* at, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->req = php::object::create<server_request>();
		self->req.native<server_request>()->init(self);
		self->req.prop("method", 6) = php::string(http_method_str((enum http_method)parser->method));
		struct http_parser_url u;
		http_parser_parse_url(at, size, 0, &u);
		if(u.field_set & (1 << UF_PATH)) {
			self->req.prop("uri",3) = php::string(at + u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
		}else{
			self->req.prop("uri",3) = php::string("/", 1);
		}
		if(u.field_set & (1 << UF_QUERY)) {
			self->query_ = php::array(0);
			// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
			// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
			kv_parser          kvparser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.on_key = kv_key_cb;
			settings.s1 = '=';
			settings.s2 = '&';
			settings.on_val = query_val_cb;
			kv_parser_init(&kvparser);
			kvparser.data = self;
			// 解析
			kv_parser_execute(&kvparser, &settings, at + u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
		}
		self->req.prop("query", 5) = std::move(self->query_);
		self->header_ = php::array(0);

		self->res = php::object::create<server_response>();
		self->res.native<server_response>()->init(self);
		return 0;
	}
	int server_connection::hp_header_field_cb(http_parser* parser, const char* at, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(self->val_.size() > 0) self->header_add();
		std::memcpy(self->key_.put(size), at, size);
		return 0;
	}
	int server_connection::hp_header_value_cb(http_parser* parser, const char* at, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->val_.put(size), at, size);
		return 0;
	}
	void server_connection::header_add() {
		for(char *c = key_.data(); c < key_.data() + key_.size(); ++c) {
			if(*c == '_') *c = '-';
			else if(*c >= 'A' && *c <= 'Z') *c = *c - 'A' + 'a';
		}
		header_.at(std::move(key_)) = std::move(val_);
	}
	int server_connection::hp_header_complete_cb(http_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(self->val_.size() > 0) self->header_add();

		php::string cookie = self->header_.at("cookie", 6);
		// is_string() 实际相当于 !is_empty() 用于检查存在性
		if(cookie.is_string()) {
			self->cookie_ = php::array(1);
			// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
			// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
			kv_parser          kvparser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.s1 = '=';
			settings.s2 = ';';
			settings.w1 = '\n';
			settings.w2 = '\r';
			settings.on_key = kv_key_cb;
			settings.on_val = cookie_val_cb;
			kv_parser_init(&kvparser);
			kvparser.data = self;
			kv_parser_execute(&kvparser, &settings, cookie.c_str(), cookie.length());
		}
		self->req.prop("cookie") = std::move(self->cookie_);
		self->req.prop("header") = std::move(self->header_);
		return 0;
	}
	int server_connection::hp_body_cb(http_parser* parser, const char* at, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->val_.put(size), at, size);
		return 0;
	}
	int server_connection::hp_message_complete(http_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(self->val_.size() == 0) {
			self->body_ = php::value(nullptr);
		}else{
			php::string raw = std::move(self->val_);
			self->req.prop("rawBody") = raw;

			// 头部信息均为小写，下划线
			php::string ctype = self->header_.at("content-type", 12);
			if(!ctype.is_string()) { // 除非 content-type 不存在，否则一定是字符串
				self->body_ = std::move(raw);
			}else if(ctype.length() >= 33 && strncmp(ctype.c_str(), "application/x-www-form-urlencoded", 33) == 0) {
				self->body_ = php::array(1);
				// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
				// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
				kv_parser          kvparser;
				kv_parser_settings settings;
				std::memset(&settings, 0, sizeof(kv_parser_settings));
				settings.s1 = '=';
				settings.s2 = '&';
				settings.on_key = kv_key_cb;
				settings.on_val = body_val_cb;
				kv_parser_init(&kvparser);
				kvparser.data = self;
				// 解析
				kv_parser_execute(&kvparser, &settings, raw.c_str(), raw.length());
			}else if(ctype.length() > 32 && strncmp(ctype.c_str(), "multipart/form-data", 19) == 0) {
				// multipart/form-data; boundary=---------xxxxxx
				self->body_ = php::array(1);
				// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
				// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
				multipart_parser          mpparser;
				multipart_parser_settings settings;
				std::memset(&settings, 0, sizeof(multipart_parser_settings));
				std::strcpy(settings.boundary, strstr(ctype.c_str() + 20, "boundary=") + 9);
				settings.boundary_length  = std::strlen(settings.boundary);
				settings.on_header_field  = mp_key_cb;
				settings.on_header_value  = mp_val_cb;
				settings.on_part_data     = mp_dat_cb;
				settings.on_part_data_end = mp_dat_end;
				multipart_parser_init(&mpparser, &settings);
				mpparser.data = self;
				// 解析
				multipart_parser_execute(&mpparser, &settings, raw.c_str(), raw.length());
				return 0;
			}else if(ctype.length() >= 16 && strncmp(ctype.c_str(), "application/json", 16) == 0) {
				self->body_ = php::json_decode(raw.c_str(), raw.length());
			}else{ // unknown
				self->body_ = std::move(raw); // 实际 self->body_ 和 prop("rawBody") 引用同一份数据
			}
			self->req.prop("body", 4) = std::move(self->body_);
		}
		// 交由 PHP 处理请求
		self->on_session(self);
		return 0;
	}
	int server_connection::mp_key_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->key_data = const_cast<char*>(at); // 由于这里数据已经完整且在解析过程中始终存在，
		self->key_size  = length; // 仅作引用，不做复制
		return 0;
	}
	int server_connection::mp_val_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(self->key_size != 19 || strncmp(self->key_data, "Content-Disposition", 19) != 0 ||
			length < 10 || strncmp(at, "form-data;", 10) != 0) {
			return 0;
		}
		// 解析此 header 行，并获取 name 字段作为实际此项数据的 KEY
		self->body_item = php::array(1);

		kv_parser          kvparser;
		kv_parser_settings settings;
		std::memset(&settings, 0, sizeof(kv_parser_settings));
		settings.s1 = '=';
		settings.s2 = ';';
		settings.w1 = '"';
		settings.w2 = '"';
		settings.on_key = kv_key_cb;
		settings.on_val = header_val_cb;
		kv_parser_init(&kvparser);
		kvparser.data = self;
		// 解析
		kv_parser_execute(&kvparser, &settings, at + 10, length);
		php::string name = self->body_item.at("name", 4);
		if(name.is_string()) {
			std::memcpy(self->key_.put(name.length()), name.c_str(), name.length());
		}
		// self->val_.reset();
		return 0;
	}
	int server_connection::mp_dat_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(length == 1) {
			self->val_.add(*at);
		}else{
			std::memcpy(self->val_.put(length), at, length);
		}
		return 0;
	}
	int server_connection::mp_dat_end(multipart_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string key = std::move(self->key_);
		php::string val = std::move(self->val_);
		self->body_.at(key) = val;
		return 0;
	}
	int server_connection::kv_key_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->key_data = at; // 由于这里数据已经完整且在解析过程中始终存在，
		self->key_size = length; // 仅作引用，不做复制
		return 0;
	}
	int server_connection::header_val_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->body_item.at(self->key_data, self->key_size) = php::string(at, length);
		return 0;
	}
	int server_connection::query_val_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->query_.at(self->key_data, self->key_size) = php::url_decode(at, length);
		return 0;
	}
	int server_connection::cookie_val_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->cookie_.at(self->key_data, self->key_size) = php::url_decode(at, length);
		return 0;
	}
	int server_connection::body_val_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->body_.at(self->key_data, self->key_size) = php::url_decode(at, length);
		return 0;
	}
}
}
}
