#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../log/log.h"
#include "server_connection_base.h"

namespace flame {
namespace net {
namespace http {
	server_connection_base::server_connection_base(void* ptr)
	: on_session(nullptr)
	, data(ptr)
	, refer_(0)
	, is_closing(false) {

	}
	// 用于在 response 中关闭连接
	void server_connection_base::close() {
		// 下述条件满足时，销毁 connection 对象
		// 1. 由 response 进行关闭时，所有关联 response 全部引用结束
		// 2. 无 response 引用
		if(--refer_ == 0)	{
			uv_read_stop(&sock_);
			is_closing = true;
			uv_close((uv_handle_t*)&sock_, close_cb);
		}
	}
	void server_connection_base::close_ex() {
		uv_read_stop(&sock_);
		is_closing = true;
		if(refer_ == 0)	{
			uv_close((uv_handle_t*)&sock_, close_cb);
		}
	}
	typedef struct write_request_t {
		coroutine*                co;
		server_connection_base* self;
		php::string             data;
		uv_write_t               req;
	} write_request_t;
	bool server_connection_base::write(const php::string& str, coroutine* co, bool silent) {
		if(is_closing) {
			if(!silent) log::default_logger->write("(WARN) write failed: connection already closed");
			return false;
		}

		write_request_t* ctx = new write_request_t {
			co, this, str
		};
		ctx->req.data = ctx;
		uv_buf_t data{ ctx->data.data(), ctx->data.length() };
		uv_write(&ctx->req, &sock_, &data, 1, write_cb);
		return true;
	}
	void server_connection_base::start() {
		sock_.data = this;
		if(uv_read_start(&sock_, alloc_cb, read_cb) < 0) close_ex();
	}
	ssize_t server_connection_base::parse(const char* data, ssize_t size) {
		return size;
	}
	void server_connection_base::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		static char buffer[16 * 1024];
		buf->base = buffer;
		buf->len  = sizeof(buffer);
	}
	void server_connection_base::read_cb (uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
		server_connection_base* self = reinterpret_cast<server_connection_base*>(handle->data);
		if(nread == UV_EOF) {
			self->close_ex();
		}else if(nread < 0) {
			log::default_logger->write(fmt::format("(WARN) http read failed: ({0}) {1}", nread, uv_strerror(nread)));
			self->close_ex();
		}else if(self->parse(buf->base, nread) == nread) {
			// continue
		}else{
			log::default_logger->write("(WARN) http parse failed: illegal data");
			self->close_ex();
		}
	}
	void server_connection_base::write_cb(uv_write_t* handle, int status) {
		write_request_t* ctx = reinterpret_cast<write_request_t*>(handle->data);
		if(status == UV_ECANCELED || status == 0) {
			if(ctx->co) ctx->co->next(php::BOOL_YES);
		}else{
			if(ctx->co) ctx->co->next(php::BOOL_NO);
			log::default_logger->write(fmt::format("(WARN) http write failed: ({0}) {1}", status, uv_strerror(status)));
		}
		delete ctx;
	}
	void server_connection_base::close_cb(uv_handle_t* handle) {
		server_connection_base* self = reinterpret_cast<server_connection_base*>(handle->data);
		// self->on_close(self, self->data);
		delete self;
	}
}
}
}
