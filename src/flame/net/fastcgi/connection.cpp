#include "../../fiber.h"
#include "connection.h"

namespace flame {
namespace net {
namespace fastcgi {
	void connection::start() {
		std::printf("connection::start()\n");
		status_ = PS_RECORD_VERSION;
		socket_.data = this;
		if(0 > uv_read_start(reinterpret_cast<uv_stream_t*>(&socket_),
			alloc_cb, read_cb)) {
			uv_close(reinterpret_cast<uv_handle_t*>(&socket_),
				close_cb);
		}
	}
	void connection::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		connection* self = reinterpret_cast<connection*>(handle->data);
		buf->base = self->buffer_; // 使用内置 buffer
		buf->len  = sizeof(self->buffer_);
	}
	void connection::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		connection* self = reinterpret_cast<connection*>(stream->data);
		if(nread < 0) {
			if(nread != UV_EOF) {
				// TODO 记录发生的错误信息日志？
			}
			uv_close(
				reinterpret_cast<uv_handle_t*>(&self->socket_),
				close_cb);
		}else if(nread == 0) {
			// again
		}else if(nread != self->parse(buf->base, nread)) {
			// TODO 记录发生的解析错误日志？
			uv_close(
				reinterpret_cast<uv_handle_t*>(&self->socket_),
				close_cb);
		}
	}

#define RECORD_ENDING() if(plen_ == 0) { \
		status_ = PS_RECORD_VERSION; \
	}else{ \
		status_ = PS_RECORD_PADDING; \
	}

	int connection::parse(const char* data, int size) {
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
				status_ = PS_RECORD_REQUEST_ID_2;
				break;
			case PS_RECORD_REQUEST_ID_2:
				status_ = PS_RECORD_CONTENT_LEN_1;
				break;
			case PS_RECORD_CONTENT_LEN_1:
				clen_ = c << 8;
				status_ = PS_RECORD_CONTENT_LEN_2;
				break;
			case PS_RECORD_CONTENT_LEN_2:
				clen_ |= c & 0xff;
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
						// status_ = PS_BODY_2_KEY_LEN_1;
						status_ = PS_BODY_3_DATA;
						break;
					case PT_STDIN:
						status_ = PS_BODY_3_DATA;
						break;
					default:
						// TODO 其他类型如何处理？
						std::printf("error: unknown fastcgi type\n");
					}
				}else{ // 无内容数据一般同时没有填充数据
					status_ = PS_RECORD_VERSION;
					std::printf("type: %d role: %d flag: %d\n", type, role, flag);
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
				std::printf("type: %d role: %d flag: %d\n", type, role, flag);
				RECORD_ENDING();
				break;
			case PS_BODY_2_KEY_LEN_1:
				--clen_;
				klen_ = (c & 0x7f) << 24;
				if(c & 0x80) {
					status_ = PS_BODY_2_KEY_LEN_2;
				}else{
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
				vlen_ = (c & 0x7f) << 24;
				if(c & 0x80) {
					status_ = PS_BODY_2_VAL_LEN_2;
				}else{
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
				klen_ |= (c & 0xff) << 8;
				status_ = PS_BODY_2_VAL_LEN_4;
				break;
			case PS_BODY_2_VAL_LEN_4:
				--clen_;
				klen_ |= c & 0xff;
				status_ = PS_BODY_2_KEY_DATA;
				break;
			case PS_BODY_2_KEY_DATA:
				--clen_;
				--klen_;
				*key_.put(1) = c;
				if(klen_ == 0) {
					if(vlen_ > 0) {
						status_ = PS_BODY_2_VAL_DATA;
					}else if(clen_ == 0) {
						RECORD_ENDING();
					}
				}
				break;
			break;
			case PS_BODY_2_VAL_DATA:
				--clen_;
				--vlen_;
				if(vlen_ == 0) {
					RECORD_ENDING();
				}
				break;
			break;
			case PS_BODY_3_DATA:
				*val_.put(1) = c;
				if(--clen_ == 0) {
					std::printf("type: %d role: %d flag: %d [%d]\n", type, role, flag, val_.size());
					RECORD_ENDING();
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
	void connection::close_cb(uv_handle_t* handle) {
		connection* self = reinterpret_cast<connection*>(handle->data);
		delete self;
	}
}
}
}

// #include "vendor.h"
// #include "fastcgi_session.h"

// fastcgi_session::fastcgi_session(mill_unixsock sock)
// :sock_(sock) {

// }

// void fastcgi_session::run() {
// 	size_t nbytes;
// READ_NEXT:
// 	nbytes = mill_unixrecv(sock_, &head_, sizeof(head_), -1);
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	if(head_.version != FCGI_VERSION_1) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	head_.content_length = ntohs(head_.content_length);
// 	nbytes = mill_unixrecv(sock_, body_, head_.content_length + head_.padding_length, -1);
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		return;
// 	}
// 	switch(head_.type) {
// 	case FCGI_BEGIN_REQUEST:
// 		this->on_begin_request();
// 	break;
// 	case FCGI_PARAMS:
// 		this->on_params();
// 	break;
// 	case FCGI_STDIN:
// 		this->on_stdin();
// 	break;
// 	default:
// 		std::printf("unknown type: %d\n", head_.type);
// 	}
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	goto READ_NEXT;
// DESTROY:
// 	mill_unixclose(sock_);
// 	delete this;
// }
// void fastcgi_session::on_begin_request() {
// 	FCGI_BeginRequestBody* body = reinterpret_cast<FCGI_BeginRequestBody*>(body_);
// 	flag_ = body->flags;
// }
// void fastcgi_session::on_params() {
	
// }
// void fastcgi_session::on_stdin() {
// 	if(head_.content_length == 0) { // 请求结束了

// 		head_.type = FCGI_STDOUT;
// 		head_.content_length = htons(40);
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);
// 		mill_unixsend(sock_, "Content-type: text/plain\r\n\r\nHello World!", 40, -1);
		
// 		head_.type = FCGI_STDOUT;
// 		head_.content_length = 0;
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);

// 		head_.type = FCGI_END_REQUEST;
// 		head_.content_length = htons( sizeof(FCGI_EndRequestBody) );
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);

// 		FCGI_EndRequestBody* body = reinterpret_cast<FCGI_EndRequestBody*>(body_);
// 		body->app_status      = htons(0);
// 		body->protocol_status = FCGI_REQUEST_COMPLETE;
// 		body->reserved[0] = '\0';
// 		body->reserved[0] = '\0';
// 		body->reserved[0] = '\0';	
// 		mill_unixsend(sock_, body, sizeof(FCGI_EndRequestBody), -1);

// 		mill_unixflush(sock_, -1);
// 		if(!(flag_ & FCGI_KEEP_CONN)) {
// 			mill_unixclose(sock_);
// 		}
// 	}
// }