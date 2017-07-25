#include "../fiber.h"
#include "stream_server.h"

namespace flame {
namespace net {
	stream_server::stream_server(uv_stream_t* s)
	: status_(0)
	, pstream_(s) {
		
	}
	php::value stream_server::run(php::parameters& params) {
		if(status_ < 2) {
			throw php::exception("failed to run server: not binded or missing handler");
		}
		pstream_->data = flame::this_fiber(this);
		int error = uv_listen(reinterpret_cast<uv_stream_t*>(pstream_), 1024, connection_cb);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		++status_;
		return flame::async;
	}
	php::value stream_server::__destruct(php::parameters& params) {
		if(status_ >= 2 && !uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(pstream_), nullptr);
		}
	}
	php::value stream_server::close(php::parameters& params) {
		if(uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) return nullptr;
		pstream_->data = flame::this_fiber(this);
		uv_close(reinterpret_cast<uv_handle_t*>(pstream_), close_cb);
		return nullptr;
	}
	void stream_server::close_cb(uv_handle_t* handle) {
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(handle->data);
		stream_server* self = f->context<stream_server>();
		if(self->status_ == 3) {
			--self->status_;
			f->context<stream_server>();
		}
		f->next(); // 这里，将当前 server 的运行协程（阻塞在 yield run()）恢复执行
	}
	void stream_server::connection_cb(uv_stream_t* stream, int error) {
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(stream->data);
		stream_server* self = f->context<stream_server>();

		if(error < 0) {
			--self->status_;
			f->context<stream_server>();
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		uv_stream_t* s = self->create_stream();
		error = uv_accept(stream, s);
		if(error < 0) {
			--self->status_;
			f->context<stream_server>();
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		// 子类实现
		self->accept(s);
	}
}
}
