#include "../../coroutine.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "../http/server_request.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	server_connection::server_connection(handler_t* svr)
	: svr_(svr)
	, query_(0)
	, header_(2)
	, cookie_(0) {}

	void server_connection::start() {
		memset(&fps_, 0, sizeof(fps_));
		// !!! fastcgi 解析过程中，数据并不一定完整，需要缓存 !!!
		fps_.on_begin_request    = fp_begin_request_cb;
		fps_.on_param_key        = fp_param_key_cb;
		fps_.on_param_val        = fp_param_val_cb;
		fps_.on_end_param        = fp_end_param_cb;
		fps_.on_data             = fp_data_cb;
		fps_.on_end_request      = fp_end_request_cb;
		fastcgi_parser_init(&fpp_);
		fpp_.data   = this;
		socket_.data = this;
		if(0 > uv_read_start(&socket_, alloc_cb, read_cb)) {
			close();
		}
	}
	void server_connection::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		static char buffer[36 * 1024];
		buf->base = buffer;
		buf->len  = sizeof(buffer);
	}
	void server_connection::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		server_connection* self = reinterpret_cast<server_connection*>(stream->data);
		if(nread < 0) {
			if(nread == UV_ECANCELED) {
				
			}else{
				self->close();
			}
		}else if(nread == 0) {
			// again
		}else if(nread != fastcgi_parser_execute(&self->fpp_, &self->fps_, buf->base, nread)) {
			std::fprintf(stderr, "error: fastcgi parse failed\n");
			self->close();
		}
	}
	int server_connection::fp_begin_request_cb(fastcgi_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->req_ = php::object::create<http::server_request>();
		self->res_ = php::object::create<fastcgi::server_response>();
		self->res_.prop("header") = php::array(0);

		fastcgi::server_response* res = self->res_.native<fastcgi::server_response>();
		res->conn_ = self;
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
			self->req_.prop("method") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "REQUEST_URI", 11) == 0) {
			self->req_.prop("uri") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "QUERY_STRING", 12) == 0) {
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
			php::strtolower_inplace(kdata, ksize);
			for(char *c = kdata; c < kdata + ksize; ++c) {
				if(*c == '_') *c = '-';
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
		self->req_.prop("rawBody") = raw;
		// 头部信息均为小写，下划线
		php::string ctype = self->header_.at("content-type", 12);
		// urlencoded
		if(ctype.is_string() && ctype.length() >= 33
			&& strncmp(ctype.c_str(), "application/x-www-form-urlencoded", 33) == 0) {

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
			return 0;
		}
		// multipart/form-data; boundary=---------xxxxxx
		if(ctype.is_string() && ctype.length() > 32
			&& strncmp(ctype.c_str(), "multipart/form-data", 19) == 0) {

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
		}
		// unknown
		self->body_ = raw;
		return 0;
	}
	int server_connection::fp_end_request_cb(fastcgi_parser* parser) {
		fp_end_data_cb(parser);
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string cookie = self->header_.at("cookie", 6);
		if(cookie.is_string()) {
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
		self->req_.prop("query")  = std::move(self->query_);
		self->req_.prop("header") = std::move(self->header_);
		self->req_.prop("cookie") = std::move(self->cookie_);
		self->req_.prop("body")   = std::move(self->body_);

		self->svr_->on_request(self->req_, self->res_);
		return 0;
	}
	void server_connection::close() {
		if(svr_ == nullptr) return;
		svr_ = nullptr;
		res_.native<server_response>()->body_sent = true;
		uv_close(
			reinterpret_cast<uv_handle_t*>(&socket_),
			close_cb);
	}
	void server_connection::close_cb(uv_handle_t* handle) {
		server_connection* self = reinterpret_cast<server_connection*>(handle->data);
		delete self;
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
