#include "stream_socket.h"
#include "net.h"
#include "../fiber.h"

namespace flame {
namespace net {
	stream_socket::stream_socket(uv_stream_t* s)
	: pstream_(s)
	, rbuffer_(1792) {
	}
	php::value stream_socket::__destruct(php::parameters& params) {
		if(!uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(pstream_), nullptr);
		}
		return nullptr;
	}
	php::value stream_socket::read(php::parameters& params) {
		pstream_->data = flame::this_fiber(this);
		int error = uv_read_start(pstream_, alloc_cb, read_cb);
		if(error < 0) { // 同步过程发生错误，直接抛出异常
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async;
	}
	void stream_socket::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		stream_socket* self = f->context<stream_socket>();
		self->rbuffer_.rev(1792);
		buf->base = self->rbuffer_.data(); // 使用内置 buffer
		buf->len  = 1792;
	}
	void stream_socket::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(stream->data);
		stream_socket* self = f->context<stream_socket>();
		if(nread < 0) {
			f->context<stream_socket>();
			if(nread == UV_EOF) {
				f->next();
			}else{ // 异步过程发生错误，需要通过协程返回
				f->next(php::make_exception(uv_strerror(nread), nread));
			}
		}else if(nread == 0) {
			// again
		}else{
			// uv_read_stop 需要在 done 之前，否则可能会导致下一个 read 停止
			uv_read_stop(stream);
			self->rbuffer_.put(nread);

			f->context<stream_socket>();
			f->next(std::move(self->rbuffer_));
		}
	}
	php::value stream_socket::write(php::parameters& params) {
		if(!params[0].is_string()) {
			throw php::exception("failed to write: only string is allowed", UV_ECHARSET);
		}
		wbuffer_ = params[0]; // 保留对数据的引用（在回调前有效）
		if(wbuffer_.length() <= 0) {
			return nullptr;
		}
		uv_write_t* req = new uv_write_t;
		req->data = flame::this_fiber(this);
		uv_buf_t buf {.base = wbuffer_.data(), .len = wbuffer_.length()};
		int error = uv_write(req, pstream_, &buf, 1, write_cb);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async;
	}
	void stream_socket::write_cb(uv_write_t* req, int status) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		stream_socket* self = f->context<stream_socket>();
		delete req;
		self->wbuffer_.reset(); // 删除引用的数据
		if(status < 0) {
			f->next(php::make_exception(uv_strerror(status), status));
		}else{
			f->next();
		}
	}

	php::value stream_socket::close(php::parameters& params) {
		if(uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) return nullptr;
		pstream_->data = flame::this_fiber();
		uv_close(reinterpret_cast<uv_handle_t*>(pstream_), close_cb);
		return nullptr;
	}
	void stream_socket::close_cb(uv_handle_t* handle) {
		flame::fiber* f = reinterpret_cast<flame::fiber*>(handle->data);
		f->next(); // 这里，将当前 server 的运行协程（阻塞在 yield run()）恢复执行
	}
}
}