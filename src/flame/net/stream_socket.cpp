#include "../fiber.h"
#include "stream_socket.h"

namespace flame {
namespace net {
	stream_socket::stream_socket(uv_stream_t* s)
	: pstream_(s) {

	}
	stream_socket::~stream_socket() {
		if(!uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(pstream_), nullptr);
		}
	}
	php::value stream_socket::read(php::parameters& params) {
		if(params.length() > 0) {
			rparam_ = params[0];
		}else{
			rparam_ = nullptr;
		}
		php::string rv = rparam_emit();
		if(!rv.is_null()) return rv;
		// 开始读取
		pstream_->data = flame::this_fiber()->push(this);
		int error = uv_read_start(pstream_, alloc_cb, read_cb);
		if(error < 0) { // 同步过程发生错误，直接抛出异常
			flame::this_fiber()->throw_exception(uv_strerror(error), error);
		}
		return flame::async();
	}

	php::string stream_socket::rparam_emit() {
		if(rparam_.is_null()) {
			return rbuffer_.get();
		}else if(rparam_.is_string()) {
			php::string& delim = rparam_;
			int n = rbuffer_.find(delim.c_str(), delim.length());
			if(n > -1) {
				return rbuffer_.get(n);
			}
		}else if(rparam_.is_long()){
			int n = rparam_;
			if(rbuffer_.size() >= n) {
				return rbuffer_.get(n);
			}
		}
		return php::string();
	}

	void stream_socket::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		stream_socket* self = f->context<stream_socket>();
		buf->base = self->rbuffer_.rev(1792);
		buf->len  = 1792;
	}
	void stream_socket::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(stream->data);
		stream_socket* self = f->context<stream_socket>();
		if(nread < 0) {
			f->context<stream_socket>();
			if(nread == UV_EOF || nread == UV_ECANCELED) {
				f->next();
			}else{ // 异步过程发生错误，需要通过协程返回
				f->next(php::make_exception(uv_strerror(nread), nread));
			}
		}else if(nread == 0) {
			// again
		}else{
			self->rbuffer_.adv(nread);
			php::string rv = self->rparam_emit();
			if(!rv.is_null()) { // 符合了
				// uv_read_stop 需要在 done 之前，否则可能会导致下一个 read 停止
				uv_read_stop(stream);
				f->next(std::move(rv));
			}
			// again
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
		req->data = flame::this_fiber()->push(this);
		uv_buf_t buf {.base = wbuffer_.data(), .len = wbuffer_.length()};
		int error = uv_write(req, pstream_, &buf, 1, write_cb);
		if(error < 0) {
			flame::this_fiber()->throw_exception(uv_strerror(error), error);
		}
		return flame::async();
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
		pstream_->data = flame::this_fiber()->push();
		uv_close(reinterpret_cast<uv_handle_t*>(pstream_), close_cb);
		return flame::async();
	}
	void stream_socket::close_cb(uv_handle_t* handle) {
		flame::fiber* f = reinterpret_cast<flame::fiber*>(handle->data);
		f->next();
	}
}
}
