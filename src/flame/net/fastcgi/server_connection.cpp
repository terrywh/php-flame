#include "../../fiber.h"
#include "server_connection.h"
#include "fastcgi.h"
#include "server.h"
#include "../http/server_request.h"

namespace flame {
namespace net {
namespace fastcgi {
	void server_connection::start() {
		status_ = PS_RECORD_VERSION;
		memset(&mps_, 0, sizeof(mps_));
		memset(&kps_, 0, sizeof(kps_));
		// !!! 由于数据集已经完整，故仅需要对应的数据函数回调即可
		mps_.on_header_field     = mp_key_cb;
		mps_.on_header_value     = mp_val_cb;
		mps_.on_part_data        = mp_dat_cb;
		kps_.on_key = kv_key_cb;
		kps_.on_val = kv_val_cb;
		socket_.data = this;
		if(0 > uv_read_start(&socket_, alloc_cb, read_cb)) {
			uv_close(reinterpret_cast<uv_handle_t*>(&socket_),
				close_cb);
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
				uv_close(reinterpret_cast<uv_handle_t*>(&self->socket_),close_cb);
			}
		}else if(nread == 0) {
			// again
		}else if(nread != self->parse(buf->base, nread)) {
			// TODO 记录发生的解析错误日志？
			uv_close(
				reinterpret_cast<uv_handle_t*>(&self->socket_),
				close_cb);
		}
	}

#define STATUS_RECORD_ENDING() if(plen_ == 0) { \
		status_ = PS_RECORD_VERSION; \
	}else{ \
		status_ = PS_RECORD_PADDING; \
	}

	int server_connection::parse(const char* data, int size) {
		char c;
		for(int i=0;i<size;++i) {
			c = data[i];
			switch(status_) {
			case PS_RECORD_VERSION:
				if(c != PV_VERSION) {
					goto PARSE_FAILED;
				}
				status_ = PS_RECORD_TYPE;
			break;
			case PS_RECORD_TYPE:
				if(c >= PT_UNKNOWN_TYPE) {
					goto PARSE_FAILED;
				}
				type = c;
				status_ = PS_RECORD_REQUEST_ID_1;
			break;
			// 目前没有实现 multiplex 机制（参照 PHP/NGINX 当前版本）
			case PS_RECORD_REQUEST_ID_1:
				request_id = c; // !!! 这里不旋转字节序，方便返回
				status_ = PS_RECORD_REQUEST_ID_2;
			break;
			case PS_RECORD_REQUEST_ID_2:
				request_id += c << 8; // !!! 这里不旋转字节序，方便返回
				status_ = PS_RECORD_CONTENT_LEN_1;
			break;
			case PS_RECORD_CONTENT_LEN_1:
				clen_ = c << 8;
				status_ = PS_RECORD_CONTENT_LEN_2;
			break;
			case PS_RECORD_CONTENT_LEN_2:
				clen_ += c & 0xff;
				status_ = PS_RECORD_PADDING_LEN;
			break;
			case PS_RECORD_PADDING_LEN:
				plen_ = c;
				status_ = PS_RECORD_RESERVED;
			break;
			case PS_RECORD_RESERVED:
				if(clen_ > 0) {
					switch(type) {
					case PT_BEGIN_REQUEST:
						status_ = PS_BODY_1_ROLE_1;
					break;
					case PT_PARAMS:
						status_ = PS_BODY_2_KEY_LEN_1;
					break;
					case PT_STDIN:
						status_ = PS_BODY_3_DATA;
					break;
					default:
						// TODO 其他类型如何处理？
						std::printf("error: unknown fastcgi type\n");
					}
				}else{ // 无内容数据一般同时没有填充数据
					switch(type) {
					case PT_PARAMS:
					break;
					case PT_STDIN: {
						// 头部信息均为小写，下划线
						php::string* ctype = reinterpret_cast<php::string*>(hdr_->find("content_type", 12));
						if(ctype == nullptr) {
							obj_.prop("body") = std::move(val_);
						}else if(ctype->length() >= 33
							&& strncmp(ctype->data(), "application/x-www-form-urlencoded", 33) == 0) {
							// 针对标准类型进行解析，方便处理
							obj_.prop("body") = php::array(0);
							bdy_ = &static_cast<php::array&>(obj_.prop("body"));
							kps_.s1 = '=';
							kps_.s2 = '&';
							kv_parser_init(&kpp_, &kps_);
							kpp_.data = this;
							php::buffer body = std::move(val_);
							kv_parser_execute(&kpp_, &kps_, body.data(), body.size());
							bdy_ = nullptr;
							// multipart/form-data; boundary=---------xxxxxx
						}else if(ctype->length() > 32 && strncmp(ctype->data(), "multipart/form-data", 19) == 0) {
							obj_.prop("body") = php::array(0);
							bdy_ = &static_cast<php::array&>(obj_.prop("body"));
							std::strcpy(mps_.boundary, strstr(ctype->data() + 20, "boundary=") + 9);
							mps_.boundary_length = std::strlen(mps_.boundary);
							multipart_parser_init(&mpp_, &mps_);
							mpp_.data = this;
							php::buffer data = std::move(val_);
							multipart_parser_execute(&mpp_, &mps_, data.data(), data.size());
							bdy_ = nullptr;
						}else{ // raw body
							obj_.prop("body") = std::move(val_);
						}

						key_.reset();
						val_.reset();
						php::string* cookie = reinterpret_cast<php::string*>(hdr_->find("cookie", 6));
						if(cookie != nullptr) {
							obj_.prop("cookie") = php::array(0);
							bdy_ = &static_cast<php::array&>(obj_.prop("cookie"));
							kps_.s1 = '=';
							kps_.s2 = ';';
							kv_parser_init(&kpp_, &kps_);
							kpp_.data = this;
							kv_parser_execute(&kpp_, &kps_, cookie->data(), cookie->length());
							bdy_ = nullptr;
						}
						hdr_ = nullptr;
						server_->on_request(this, std::move(obj_));
					}
					break;
					default:
						// TODO 其他类型如何处理？
						std::printf("error: unknown fastcgi type\n");
					}
					status_ = PS_RECORD_VERSION;
				}
			break;
			case PS_BODY_1_ROLE_1:
				--clen_;
				role = c << 8;
				status_ = PS_BODY_1_ROLE_2;
			break;
			case PS_BODY_1_ROLE_2:
				--clen_;
				role |= c & 0xff;
				// 目前仅支持一种角色
				if(role != PR_RESPONDER) {
					goto PARSE_FAILED;
				}
				status_ = PS_BODY_1_FLAGS;
			break;
			case PS_BODY_1_FLAGS:
				--clen_;
				flag = c;
				status_ = PS_BODY_1_RESERVED_1;
			break;
			case PS_BODY_1_RESERVED_1:
			case PS_BODY_1_RESERVED_2:
			case PS_BODY_1_RESERVED_3:
			case PS_BODY_1_RESERVED_4:
				--clen_;
				++status_;
			break;
			case PS_BODY_1_RESERVED_5:
				--clen_;
				obj_ = php::object::create<flame::net::http::server_request>();
				hdr_ = &static_cast<php::array&>(obj_.prop("header"));
				STATUS_RECORD_ENDING();
			break;
			case PS_BODY_2_KEY_LEN_1:
				--clen_;
				if(c & 0x80) {
					klen_ = (c & 0x7f) << 24;
					status_ = PS_BODY_2_KEY_LEN_2;
				}else{
					klen_ = c;
					status_ = PS_BODY_2_VAL_LEN_1;
				}
			break;
			case PS_BODY_2_KEY_LEN_2:
				--clen_;
				klen_ |= (c & 0xff) << 16;
				status_ = PS_BODY_2_KEY_LEN_3;
			break;
			case PS_BODY_2_KEY_LEN_3:
				--clen_;
				klen_ |= (c & 0xff) << 8;
				status_ = PS_BODY_2_KEY_LEN_4;
			break;
			case PS_BODY_2_KEY_LEN_4:
				--clen_;
				klen_ |= c & 0xff;
				status_ = PS_BODY_2_VAL_LEN_1;
			break;
			case PS_BODY_2_VAL_LEN_1:
				--clen_;
				if(c & 0x80) {
					vlen_ = (c & 0x7f) << 24;
					status_ = PS_BODY_2_VAL_LEN_2;
				}else{
					vlen_ = c;
					status_ = PS_BODY_2_KEY_DATA;
				}
			break;
			case PS_BODY_2_VAL_LEN_2:
				--clen_;
				klen_ |= (c & 0xff) << 16;
				status_ = PS_BODY_2_VAL_LEN_3;
			break;
			case PS_BODY_2_VAL_LEN_3:
				--clen_;
				vlen_ |= (c & 0xff) << 8;
				status_ = PS_BODY_2_VAL_LEN_4;
			break;
			case PS_BODY_2_VAL_LEN_4:
				--clen_;
				vlen_ |= c & 0xff;
				status_ = PS_BODY_2_KEY_DATA;
			break;
			case PS_BODY_2_KEY_DATA:
				--clen_;
				--klen_;
				*key_.put(1) = c;
				if(klen_ == 0) { // KEY 接收完毕
					if(clen_ == 0) { // 所有 BODY 结束（不存在 VALUE）
						STATUS_RECORD_ENDING();
					}else if(vlen_ > 0) { // 存在 VALUE 要接收
						status_ = PS_BODY_2_VAL_DATA;
					}else{ // 无 value 直接下一个 KEY 接收
						status_ = PS_BODY_2_KEY_LEN_1;
						key_.reset();
					}
				}
			break;
			case PS_BODY_2_VAL_DATA:
				--clen_;
				--vlen_;
				*val_.put(1) = c;
				if(vlen_ == 0) { // VALUE 接收完毕
					if(strncmp(key_.data(), "REQUEST_METHOD", 14) == 0) {
						key_.reset();
						obj_.prop("method") = std::move(val_);
					}else if(strncmp(key_.data(), "REQUEST_URI", 11) == 0) {
						key_.reset();
						obj_.prop("uri") = std::move(val_);
					}else if(strncmp(key_.data(), "QUERY_STRING", 12) == 0) {
						key_.reset();
						obj_.prop("query") = php::array(0);
						bdy_ = &static_cast<php::array&>(obj_.prop("query"));
						kps_.s1 = '=';
						kps_.s2 = '&';
						kv_parser_init(&kpp_, &kps_);
						kpp_.data = this;
						php::buffer data = std::move(val_);
						kv_parser_execute(&kpp_, &kps_, data.data(), data.size());
						bdy_ = nullptr;
					}else if(strncmp(key_.data(), "HTTP_", 5) == 0) {
						php::strtolower_inplace(key_.data() + 5, key_.size() - 5);
						hdr_->at(key_.data() + 5, key_.size() - 5) = std::move(val_);
						key_.reset();
					}else {
						val_.reset();
						key_.reset();
					}
					// val_ 已经 std::move() 无需 reset
					if(clen_ == 0) { // 所有 BODY 结束
						STATUS_RECORD_ENDING();
					}else{ // 下个 KEY 长度
						status_ = PS_BODY_2_KEY_LEN_1;
					}
				}
			break;
			case PS_BODY_3_DATA:
				*val_.put(1) = c;
				if(--clen_ == 0) {
					STATUS_RECORD_ENDING();
				}
			break;
			case PS_RECORD_PADDING:
				if(--plen_ == 0) {
					status_ = PS_RECORD_VERSION;
				}
			break;
			}
			continue;
PARSE_FAILED:
		break;
		}
		return size;
	}
	void server_connection::close() {
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
		self->key_.reset();
		std::memcpy(self->key_.put(length), at, length); // !!! TODO 避免拷贝
		return 0;
	}
	int server_connection::mp_val_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		php::string key = std::move(self->key_);
		if(key.length() != 19 || strncmp(key.c_str(), "Content-Disposition", 19) != 0 ||
			length < 10 || strncmp(at, "form-data;", 10)) {
			return 0;
		}
		php::array* old = self->bdy_;
		//
		php::array array_header(0);
		self->bdy_ = &array_header;
		self->kps_.s1 = '=';
		self->kps_.s2 = ';';
		self->kps_.w1 = '"';
		self->kps_.w2 = '"';
		kv_parser_init(&self->kpp_, &self->kps_);
		self->kpp_.data = self;
		kv_parser_execute(&self->kpp_, &self->kps_, at + 10, length);

		auto iname = array_header.find("name");
		if(iname != nullptr) {
			php::string name = *iname;
			self->key_.reset(0);
			memcpy(self->key_.put(name.length()), name.data(), name.length()); // !!! TODO 避免拷贝
		}
		//
		self->bdy_ = old;
		return 0;
	}
	int server_connection::mp_dat_cb(multipart_parser* parser, const char *at, size_t length) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		self->bdy_->at(self->key_.data(), self->key_.size()) = php::string(at, length);
		return 0;
	}
	int server_connection::kv_key_cb(kv_parser* parser, const char* at, size_t len) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->key_.put(len), at, len);
		return 0;
	}
	int server_connection::kv_val_cb(kv_parser* parser, const char* at, size_t len) {
		server_connection* self = reinterpret_cast<server_connection*>(parser->data);
		std::memcpy(self->val_.put(len), at, len);
		self->bdy_->at(self->key_.data(), self->key_.size()) = std::move(self->val_);
		self->key_.reset();
		return 0;
	}
}
}
}
