#include "tcp_socket.h"
#include "net.h"
#include "../fiber.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket()
	:rbuffer_(1792) {
		uv_tcp_init(flame::loop, &socket_);
		// socket_.data = this;
	}
	php::value tcp_socket::__destruct(php::parameters& params) {
		if(!uv_is_closing(reinterpret_cast<uv_handle_t*>(&socket_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(&socket_), nullptr);
		}
		return nullptr;
	}
	
	php::value tcp_socket::connect(php::parameters& params) {
		php::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int        error = 0;
		error = addrfrom(address, addr.c_str(), port);
		if(error != 0) {
			throw php::exception(uv_strerror(error), error);
		}
		uv_connect_t* req = new uv_connect_t;
		req->data = flame::this_fiber()->push(this);
		error = uv_tcp_connect(req, &socket_, reinterpret_cast<struct sockaddr*>(&address), connect_cb);
		if(error != 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async;
	}
	void tcp_socket::connect_cb(uv_connect_t* req, int error) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		tcp_socket* self = f->pop<tcp_socket>();
		delete req;
		
		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
		}else{
			f->next();
		}
		// store 进 fiber 中的数据
		self->init_prop();
	}
	php::value tcp_socket::read(php::parameters& params) {
		socket_.data = flame::this_fiber()->push(this);
		int error = uv_read_start(reinterpret_cast<uv_stream_t*>(&socket_), alloc_cb, read_cb);
		if(error < 0) { // 同步过程发生错误，直接抛出异常
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async;
	}
	void tcp_socket::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		tcp_socket* self = f->top<tcp_socket>();
		self->rbuffer_.rev(1792);
		buf->base = self->rbuffer_.data(); // 使用内置 buffer
		buf->len  = 1792;
	}
	void tcp_socket::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(stream->data);
		tcp_socket* self = f->top<tcp_socket>();
		if(nread < 0) {
			f->pop<tcp_socket>();
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

			f->pop<tcp_socket>();
			f->next(std::move(self->rbuffer_));
		}
	}
	php::value tcp_socket::write(php::parameters& params) {
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
		int error = uv_write(req, reinterpret_cast<uv_stream_t*>(&socket_), &buf, 1, write_cb);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async;
	}
	void tcp_socket::write_cb(uv_write_t* req, int status) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		tcp_socket* self = f->pop<tcp_socket>();
		delete req;
		self->wbuffer_.reset(); // 删除引用的数据
		if(status < 0) {
			f->next(php::make_exception(uv_strerror(status), status));
		}else{
			f->next();
		}
	}
	void tcp_socket::init_prop() {
		struct sockaddr_storage address;
		int                     addrlen = sizeof(address);
		uv_tcp_getsockname(&socket_, 
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("local_address") = addr2str(address);
		prop("local_port") = addr2int(address);
		uv_tcp_getpeername(&socket_, 
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("remote_address") = addr2str(address);
		prop("remote_port") = addr2int(address);
	}

	php::value tcp_socket::close(php::parameters& params) {
		if(uv_is_closing(reinterpret_cast<uv_handle_t*>(&socket_))) return nullptr;
		socket_.data = flame::this_fiber();
		uv_close(reinterpret_cast<uv_handle_t*>(&socket_), close_cb);
		return nullptr;
	}
	void tcp_socket::close_cb(uv_handle_t* handle) {
		flame::fiber* f = reinterpret_cast<flame::fiber*>(handle->data);
		f->next(); // 这里，将当前 server 的运行协程（阻塞在 yield run()）恢复执行
	}
}
}