#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../http/server_connection_base.h"
#include "../http/server_response_base.h"
#include "../http/server_request.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	server_connection::server_connection(void* data)
	: http::server_connection_base(data)
	, query_(nullptr)
	, header_(nullptr)
	, cookie_(nullptr) {
		memset(&fps_, 0, sizeof(fps_));
		// !!! fastcgi 解析过程中，数据并不一定完整，需要缓存 !!!
		fps_.on_begin_request    = fp_begin_request_cb;
		fps_.on_param_key        = fp_param_key_cb;
		fps_.on_param_val        = fp_param_val_cb;
		fps_.on_end_param        = fp_end_param_cb;
		fps_.on_data             = fp_data_cb;
		fps_.on_end_request      = fp_end_request_cb;
		fastcgi_parser_init(&fpp_);
		fpp_.data = this;
	}
	server_connection::~server_connection() {
		
	}
	ssize_t server_connection::parse(const char* data, ssize_t size) {
		// for(int i=0;i<size;++i) {
		// 	std::printf("%02x ", (unsigned char)data[i]);
		// }
		// std::printf("\n");
		return fastcgi_parser_execute(&fpp_, &fps_, data, size);
	}
	void server_connection::close() {
		// 当前连接请求处理完成 而且 无保持连接标记
		if(--refer_ == 0 && (fpp_.flag & FASTCGI_FLAGS_KEEP_CONN) == 0) {
			close_ex();
		}
	}
	int server_connection::fp_begin_request_cb(fastcgi_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->req = php::object::create<http::server_request>();
		self->req.native<http::server_request>()->init(self);
		self->res = php::object::create<fastcgi::server_response>();
		self->res.native<fastcgi::server_response>()->init(self);
		self->header_ = php::array(0);
		return 0;
	}
	int server_connection::fp_param_key_cb(fastcgi_parser* parser, const char* data, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->key_.put(size), data, size);
		return 0;
	}
	int server_connection::fp_param_val_cb(fastcgi_parser* parser, const char* data, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->val_.put(size), data, size);
		return 0;
	}
	int server_connection::fp_end_param_cb(fastcgi_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		char*  kdata = self->key_.data();
		size_t ksize = self->key_.size();
		char*  vdata = self->val_.data();
		size_t vsize = self->val_.size();
		// 虽然 reset 但内容数据还存在
		self->key_.reset();
		self->val_.reset();

		if(strncmp(kdata, "REQUEST_METHOD", 14) == 0) {
			php::strtoupper_inplace(vdata, vsize);
			self->req.prop("method") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "REQUEST_URI", 11) == 0) {
			self->req.prop("uri") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "QUERY_STRING", 12) == 0) {
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
			kv_parser_execute(&kvparser, &settings, vdata, vsize);
		}else if(strncmp(kdata, "HTTP_", 5) == 0) {
			for(char *c = kdata; c < kdata + ksize; ++c) {
				if(*c == '_') *c = '-';
				else if(*c >= 'A' && *c <= 'Z') *c = *c - 'A' + 'a';
			}
			self->header_.at(kdata + 5, ksize - 5) = php::string(vdata, vsize);
		}
		return 0;
	}
	int server_connection::fp_data_cb(fastcgi_parser* parser, const char* data, size_t size) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);

		std::memcpy(self->val_.put(size), data, size);
		return 0;
	}
	int server_connection::fp_end_data_cb(fastcgi_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string raw = std::move(self->val_);
		self->req.prop("rawBody") = raw;
		// 头部信息均为小写，下划线
		php::string ctype = self->header_.at("content-type", 12);
		if(!ctype.is_string()) { // 除非 content-type 不存在，否则一定是字符串
			self->body_ = std::move(raw);
			return 0;
		}
		// 除非 content-type 不存在，否则一定是字符串
		// application/x-www-form-urlencoded
		if(ctype.length() >= 33 && strncmp(ctype.c_str(), "application/x-www-form-urlencoded", 33) == 0) {
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
			self->body_ = raw;
		}
		return 0;
	}
	int server_connection::fp_end_request_cb(fastcgi_parser* parser) {
		fp_end_data_cb(parser);
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string cookie = self->header_.at("cookie", 6);
		if(cookie.is_string()) {
			self->cookie_ = php::array(0);
			kv_parser parser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.s1 = '=';
			settings.s2 = ';';
			settings.w1 = '\n';
			settings.w2 = '\r';
			settings.on_key = kv_key_cb;
			settings.on_val = cookie_val_cb;
			kv_parser_init(&parser);
			parser.data = self;
			kv_parser_execute(&parser, &settings, cookie.c_str(), cookie.length());
		}
		self->req.prop("query",5)  = std::move(self->query_);
		self->req.prop("header",6) = std::move(self->header_);
		self->req.prop("cookie",6) = std::move(self->cookie_);
		self->req.prop("body",4)   = std::move(self->body_);

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
