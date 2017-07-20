#include "net.h"
#include "../fiber.h"
#include "tcp_server.h"
#include "stream_socket.h"
#include "tcp_socket.h"


namespace flame {
namespace net {
	tcp_server::tcp_server()
	:status_(0) {
		uv_tcp_init(flame::loop, &server_);
	}
	php::value tcp_server::__destruct(php::parameters& params) {
		if(!uv_is_closing(reinterpret_cast<uv_handle_t*>(&server_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(&server_), nullptr);
		}
	}
	php::value tcp_server::handle(php::parameters& params) {
		handle_ = params[0];
		++status_;
		return nullptr;
	}
	php::value tcp_server::bind(php::parameters& params) {
		php::string& addr = params[0];
		int          port = params[1];
		struct sockaddr_storage address;
		int error = addrfrom(address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		error = uv_tcp_bind(&server_, reinterpret_cast<struct sockaddr*>(&address), 0);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		++status_;
		return nullptr;
	}
	php::value tcp_server::run(php::parameters& params) {
		if(status_ < 2) {
			throw php::exception("failed to run server: not binded or missing handler");
		}
		server_.data = flame::this_fiber()->push(this);
		int error = uv_listen(reinterpret_cast<uv_stream_t*>(&server_), 1024, connection_cb);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		++status_;
		return flame::async;
	}
	void tcp_server::connection_cb(uv_stream_t* stream, int error) {
		flame::fiber* f = reinterpret_cast<flame::fiber*>(stream->data);
		tcp_server*self = f->top<tcp_server>();

		if(error < 0) {
			--self->status_;
			f->pop<tcp_server>();
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		php::object sock = php::object::create<tcp_socket>();
		tcp_socket* scli = sock.native<tcp_socket>();
		error = uv_accept(stream, reinterpret_cast<uv_stream_t*>(&scli->socket_));
		if(error < 0) {
			--self->status_;
			f->pop<tcp_server>();
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		// 在新的“协程”中运行客户端连接的 handle 过程
		if(!fiber::start(self->handle_, sock)) {
			--self->status_;
			f->pop<tcp_server>();
			f->next(php::make_exception("unknown error", 0));
			return;
		}
	}
	php::value tcp_server::close(php::parameters& params) {
		if(uv_is_closing(reinterpret_cast<uv_handle_t*>(&server_))) return nullptr;
		server_.data = flame::this_fiber()->push(this);
		uv_close(reinterpret_cast<uv_handle_t*>(&server_), close_cb);
		return nullptr;
	}
	void tcp_server::close_cb(uv_handle_t* handle) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		tcp_server* self = f->pop<tcp_server>();
		if(self->status_ == 3) {
			--self->status_;
			f->pop<tcp_server>();
		}
		f->next(); // 这里，将当前 server 的运行协程（阻塞在 yield run()）恢复执行
	}
}
}
