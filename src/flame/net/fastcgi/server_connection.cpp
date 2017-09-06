#include "../../fiber.h"
#include "server_connection.h"
#include "fastcgi.h"
#include "server.h"
#include "../http/server_request.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	void server_connection::start() {
		memset(&fps_, 0, sizeof(fps_));
		// !!! fastcgi 解析过程中，数据并不一定完整，需要缓存 !!!
		fps_.on_begin_request    = fp_begin_request_cb;
		fps_.on_param_key        = fp_param_key_cb;
		fps_.on_param_val        = fp_param_val_cb;
		fps_.on_end_param        = fp_end_param_cb;
		fps_.on_data             = fp_data_cb;
		fps_.on_end_request      = fp_end_request_cb;
		fastcgi_parser_init(&fpp_, &fps_);
		fpp_.data    = this;
		socket_.data = this;
		if(0 > uv_read_start(&socket_, alloc_cb, read_cb)) {
			close();
		}
	}
	void server_connection::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		server_connection* self = reinterpret_cast<server_connection*>(handle->data);
		buf->base = self->buffer_; // 使用内置 buffer
		buf->len  = sizeof(self->buffer_);
	}
	void server_connection::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		server_connection* self = reinterpret_cast<server_connection*>(stream->data);
		if(nread < 0) {
			if(nread != UV_EOF) {
				// TODO 记录发生的错误信息日志？
			}else if(nread != UV_ECANCELED) {
				self->close();
			}
		}else if(nread == 0) {
			// again
		}else if(nread != fastcgi_parser_execute(&self->fpp_, &self->fps_, buf->base, nread)) {
			std::printf("error: fastcgi parse failed\n");
			self->close();
		}
	}
	int server_connection::fp_begin_request_cb(fastcgi_parser* parser) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->req_ = php::object::create<http::server_request>();
		self->req_.prop("header") = php::array(0);
		self->ctr_ = &static_cast<php::array&>(self->req_.prop("header"));
		self->res_ = php::object::create<fastcgi::server_response>();
		fastcgi::server_response* req = self->res_.native<fastcgi::server_response>();
		req->conn_ = self;
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
		self->key_.reset();
		self->val_.reset();

		if(strncmp(kdata, "REQUEST_METHOD", 14) == 0) {
			self->req_.prop("method") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "REQUEST_URI", 11) == 0) {
			self->req_.prop("uri") = php::string(vdata, vsize);
		}else if(strncmp(kdata, "QUERY_STRING", 12) == 0) {
			self->req_.prop("query") = php::array(0);
			// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
			// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
			kv_parser          kvparser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.on_key = kv_key_cb;
			settings.s1 = '=';
			settings.s2 = '&';
			settings.on_val = kv_val_cb_2;
			kv_parser_init(&kvparser, &settings);
			kvparser.data = self;
			// 解析
			php::array* ctr = self->ctr_;
			self->ctr_ = &static_cast<php::array&>(self->req_.prop("query"));
			kv_parser_execute(&kvparser, &settings, vdata, vsize);
			self->ctr_ = ctr;
		}else if(strncmp(kdata, "HTTP_", 5) == 0) {
			php::strtolower_inplace(kdata, ksize);
			self->ctr_->at(kdata + 5, ksize - 5) = php::string(vdata, vsize);
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
		// 头部信息均为小写，下划线
		php::string* ctype = reinterpret_cast<php::string*>(self->ctr_->find("content_type", 12));
		if(ctype == nullptr) {
			self->req_.prop("body") = std::move(self->val_);
			return 0;
		}
		// urlencoded
		if(ctype->length() >= 33
			&& strncmp(ctype->data(), "application/x-www-form-urlencoded", 33) == 0) {
			self->req_.prop("body") = php::array(0);
			// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
			// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
			kv_parser          kvparser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.s1 = '=';
			settings.s2 = '&';
			settings.on_key = kv_key_cb;
			settings.on_val = kv_val_cb_2;
			kv_parser_init(&kvparser, &settings);
			kvparser.data = self;
			// 解析
			php::array* ctr = self->ctr_;
			self->ctr_ = &static_cast<php::array&>(self->req_.prop("body"));
			kv_parser_execute(&kvparser, &settings, self->val_.data(), self->val_.size());
			self->val_.reset(0);
			self->ctr_ = ctr;
			return 0;
		}
		// multipart/form-data; boundary=---------xxxxxx
		if(ctype->length() > 32
			&& strncmp(ctype->data(), "multipart/form-data", 19) == 0) {

			self->req_.prop("body") = php::array(0);
			// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可 !!!
			// !!! 且待解析数据在解析过程始终存在于内存中，可直接引用 !!!
			multipart_parser          mpparser;
			multipart_parser_settings settings;
			std::memset(&settings, 0, sizeof(multipart_parser_settings));
			std::strcpy(settings.boundary, strstr(ctype->data() + 20, "boundary=") + 9);
			settings.boundary_length = std::strlen(settings.boundary);
			settings.on_header_field = mp_key_cb;
			settings.on_header_value = mp_val_cb;
			settings.on_part_data    = mp_dat_cb;
			multipart_parser_init(&mpparser, &settings);
			mpparser.data = self;
			// 解析
			php::array* ctr = self->ctr_;
			self->ctr_ = &static_cast<php::array&>(self->req_.prop("body"));
			multipart_parser_execute(&mpparser, &settings, self->val_.data(), self->val_.size());
			self->val_.reset(0);
			self->ctr_ = ctr;
			return 0;
		}
		// unknown
		self->req_.prop("body") = std::move(self->val_);
		return 0;
	}
	int server_connection::fp_end_request_cb(fastcgi_parser* parser) {
		fp_end_data_cb(parser);
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string* cookie = reinterpret_cast<php::string*>(self->ctr_->find("cookie", 6));
		if(cookie != nullptr) {
			self->req_.prop("cookie") = php::array(0);

			kv_parser parser;
			kv_parser_settings settings;
			std::memset(&settings, 0, sizeof(kv_parser_settings));
			settings.s1 = '=';
			settings.s2 = ';';
			settings.on_key = kv_key_cb;
			settings.on_val = kv_val_cb_2;
			kv_parser_init(&parser, &settings);
			parser.data = self;

			php::array* ctr = self->ctr_;
			self->ctr_ = &static_cast<php::array&>(self->req_.prop("cookie"));
			kv_parser_execute(&parser, &settings, cookie->data(), cookie->length());
			self->ctr_ = ctr;
		}
		self->svr_->on_request(self->req_, self->res_);
		self->ctr_ = nullptr;
		return 0;
	}
	void server_connection::close() {
		if(svr_ == nullptr) return;
		svr_ = nullptr;
		res_.prop("ended") = true;
		uv_close(
			reinterpret_cast<uv_handle_t*>(&socket_),
			close_cb);
	}
	void server_connection::close_cb(uv_handle_t* handle) {
		server_connection* self = reinterpret_cast<server_connection*>(handle->data);
		delete self;
	}
	static uv_buf_t cache_key;
	int server_connection::mp_key_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		cache_key.base = const_cast<char*>(at); // 由于这里数据已经完整且在解析过程中始终存在，
		cache_key.len  = length; // 仅作引用，不做复制
		return 0;
	}
	int server_connection::mp_val_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		if(cache_key.len != 19 || strncmp(cache_key.base, "Content-Disposition", 19) != 0 ||
			length < 10 || strncmp(at, "form-data;", 10) != 0) {
			return 0;
		}
		// 解析此 header 行，并获取 name 字段作为实际此项数据的 KEY
		php::array header(0);

		kv_parser          kvparser;
		kv_parser_settings settings;
		std::memset(&settings, 0, sizeof(kv_parser_settings));
		settings.s1 = '=';
		settings.s2 = ';';
		settings.w1 = '"';
		settings.w2 = '"';
		settings.on_key = kv_key_cb;
		settings.on_val = kv_val_cb_1;
		kv_parser_init(&kvparser, &settings);
		kvparser.data = self;

		php::array* ctr = self->ctr_;
		self->ctr_ = &header;
		kv_parser_execute(&kvparser, &settings, at + 10, length);
		self->ctr_ = ctr;
		php::string* name = reinterpret_cast<php::string*>(header.find("name"));
		if(name != nullptr) {
			std::memcpy(self->key_.put(name->length()), name->data(), name->length()); // !!! TODO 避免拷贝
		}
		return 0;
	}
	int server_connection::mp_dat_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->ctr_->at(self->key_.data(), self->key_.size()) = php::string(at, length);
		self->key_.reset();
		return 0;
	}
	int server_connection::kv_key_cb(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		cache_key.base = const_cast<char*>(at); // 由于这里数据已经完整且在解析过程中始终存在，
		cache_key.len  = length; // 仅作引用，不做复制
		return 0;
	}
	int server_connection::kv_val_cb_1(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->ctr_->at(cache_key.base, cache_key.len) = php::string(at, length);
		return 0;
	}
	int server_connection::kv_val_cb_2(kv_parser* parser, const char* at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->ctr_->at(cache_key.base, cache_key.len) = php::url_decode(at, length);
		return 0;
	}
}
}
}
