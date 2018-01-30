#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "stream_writer.h"

namespace flame {
namespace io {
	stream_writer::stream_writer(uv_stream_t* s)
	: cli_(s) { }
	typedef struct write_request_t {
		coroutine*       co;
		stream_writer* self;		
		php::string    data;
		uv_write_t      req;
	} write_request_t;
	void stream_writer::write(const php::string& data) {
		if(cli_ == nullptr) throw php::exception("failed to write: stream is already closed");
		
		write_request_t* ctx = new write_request_t {
			coroutine::current, this, data			
		};
		ctx->req.data = ctx;
		uv_buf_t buf {.base = ctx->data.data(), .len = ctx->data.length()};
		uv_write(&ctx->req, (uv_stream_t*)cli_, &buf, 1, write_cb);
	}
	void stream_writer::write_cb(uv_write_t* handle, int status) {
		auto ctx = static_cast<write_request_t*>(handle->data);
		if(status == UV_ECANCELED) {
			ctx->self->close();
			ctx->co->next();
		}else if(status < 0) {
			ctx->self->close();
			ctx->co->fail(uv_strerror(status), status);
		}else{
			ctx->co->next();
		}
		delete ctx;
	}
	void stream_writer::close() {
		cli_ = nullptr;
	}
}
}
