#include "../../flame.h"
#include "../../coroutine.h"
#include "messenger.h"
#include "../../net/tcp_socket.h"
#include "../../net/unix_socket.h"

namespace flame {
namespace os {
namespace cluster {
	messenger::messenger()
	: cb_type()
	, stat_(0)
	, size_(0) {
		pipe_ = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
		uv_pipe_init(flame::loop, pipe_, 1);
		uv_pipe_open(pipe_, 0);
		pipe_->data = this;
	}
	messenger::~messenger() {
		uv_close((uv_handle_t*)pipe_, free_handle_cb);
	}
	void messenger::start() {
		uv_read_start((uv_stream_t*)pipe_, alloc_cb, read_cb);
		uv_unref((uv_handle_t*)pipe_);
	}
	void messenger::alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf) {
		static char buffer[2048];
		buf->base = buffer;
		buf->len  = sizeof(buffer);
	}
	void messenger::read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
		messenger* self = static_cast<messenger*>(handle->data);
		if(nread < 0) {
			std::fprintf(stderr, "error: failed to read ipc, (%d) %s\n", nread, uv_strerror(nread));
		}else if(nread == 0) {

		}else if(nread != self->parse(buf->base, nread)){
			std::fprintf(stderr, "error: failed to read ipc, wrong format\n");
		}
	}
	size_t messenger::parse(char* data, size_t size) {
		size_t i = 0, mark = 0;
		while(i< size) {
			char c = data[i];
			switch(stat_) {
			case PPS_SIZE_0:
				size_ = c;
				stat_ = PPS_SIZE_1;
				break;
			case PPS_SIZE_1:
				size_ |= c << 8;
				if(size_ == 0) {
					stat_ = PPS_SIZE_0;
					on_socket();
				}else{
					mark = i + 1;
					stat_ = PPS_DATA;
				}
				break;
			case PPS_DATA:
				--size_;
				if(size_ == 0) {
					std::memcpy(data_.put(i-mark+1), data + mark, i-mark+1);
					stat_ = PPS_SIZE_0;
					on_string();
				}else if(i+1 == size) {
					std::memcpy(data_.put(i-mark+1), data + mark, i-mark+1);
				}
				break;
			}
			++i;
		}
		return i;
	}

	void messenger::on_socket() {
		int  type = uv_pipe_pending_type(pipe_);
		if(cb_type & 0x04) { // 允许使用内部对象接管客户端对象
			zval server;
			ZVAL_PTR(&server, pipe_); // 将服务器对象指针放入，用于 accept 连接
			coroutine::start(cb_socket, reinterpret_cast<php::value&>(server), type);
			return;
		}
		if((cb_type & 0x02) == 0) return;

		if(type == UV_TCP) {
			php::object obj = php::object::create<net::tcp_socket>();
			net::tcp_socket* cpp = obj.native<net::tcp_socket>();
			if(uv_accept((uv_stream_t*)pipe_, (uv_stream_t*)&cpp->impl->stream) < 0) {
				std::fprintf(stderr, "error: failed to accept socket from parent\n");
				return;
			}
			cpp->after_init();
			coroutine::start(cb_socket, obj);
		}else if(type == UV_NAMED_PIPE) {
			php::object obj = php::object::create<net::unix_socket>();
			net::unix_socket* cpp = obj.native<net::unix_socket>();
			if(uv_accept((uv_stream_t*)pipe_, (uv_stream_t*)&cpp->impl->stream) < 0) {
				std::fprintf(stderr, "error: failed to accept socket from parent\n");
				return;
			}
			cpp->after_init();
			coroutine::start(cb_socket, obj);
		}

	}
	void messenger::on_string() {
		if((cb_type & 0x01) == 0) return;
		php::string data = std::move(data_);
		coroutine::start(cb_string, data);
	}

	typedef struct send_request_t {
		coroutine*   co;
		php::value  ref;
		php::value  obj;
		uv_write_t  req;
	} send_request_t;
	void messenger::send_string_cb(uv_write_t* handle, int status) {
		send_request_t* ctx = static_cast<send_request_t*>(handle->data);
		if(ctx->co != nullptr) ctx->co->next();
		delete ctx;
	}
	void messenger::send_socket_cb(uv_write_t* handle, int status) {
		send_request_t* ctx = static_cast<send_request_t*>(handle->data);
		// 当前进程中的连接需要关闭
		if(static_cast<php::object&>(ctx->obj).is_instance_of<net::tcp_socket>()) {
			static_cast<php::object&>(ctx->obj).native<net::tcp_socket>()->impl->close(true);
		}else{
			static_cast<php::object&>(ctx->obj).native<net::unix_socket>()->impl->close(true);
		}
		if(ctx->co != nullptr) ctx->co->next();
		delete ctx;
	}
	void messenger::send(php::parameters& params, uv_stream_t* stream) {
		send_request_t* ctx = new send_request_t {
			.co  = stream == nullptr ? nullptr : coroutine::current,
			.ref = this,
			.obj = params[0]
		};
		ctx->req.data = ctx;
		if(stream == nullptr) stream = (uv_stream_t*)pipe_;
		if(ctx->obj.is_object() && static_cast<php::object&>(ctx->obj).is_instance_of<net::unix_socket>()) {
			uv_stream_t* ss = (uv_stream_t*)&static_cast<php::object&>(ctx->obj).native<net::unix_socket>()->impl->stream;
			uv_buf_t data {.base = (char*)"\x00\x00", .len = 2};

			uv_write2(&ctx->req, stream, &data, 1, ss, send_socket_cb);
		}else if(ctx->obj.is_object() && static_cast<php::object&>(ctx->obj).is_instance_of<net::tcp_socket>()) {
			uv_stream_t* ss = (uv_stream_t*)&static_cast<php::object&>(ctx->obj).native<net::tcp_socket>()->impl->stream;
			uv_buf_t data {.base = (char*)"\x00\x00", .len = 2};

			uv_write2(&ctx->req, stream, &data, 1, ss, send_socket_cb);
		}else{
			php::string& str = ctx->obj.to_string();
			if(str.length() > 65535 || str.length() < 1) {
				throw php::exception("failed to send message: length overflow or less than one");
			}
			short len = str.length();
			uv_buf_t data [] = {
				{ .base = (char*)&len, .len = sizeof(len)  },
				{ .base = str.data(),  .len = str.length() }
			};
			uv_write(&ctx->req, stream, data, 2, send_string_cb);
		}
	}
}
}
}
