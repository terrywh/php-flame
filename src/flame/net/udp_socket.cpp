#include "../fiber.h"
#include "net.h"
#include "udp_socket.h"

namespace flame {
namespace net {
	udp_socket::udp_socket() {
		uv_udp_init(flame::loop, &socket_);
		// socket_.data = this;
	}
	php::value udp_socket::bind(php::parameters& params) {
		std::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int error = addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// uv_udp_init_ex 会创建 socket
		uv_udp_init_ex(flame::loop, &socket_, address.ss_family);
		// 然后才能进行 SO_REUSEPORT 设置
		enable_socket_reuseport(reinterpret_cast<uv_handle_t*>(&socket_));
		error = uv_udp_bind(&socket_, reinterpret_cast<struct sockaddr*>(&address), UV_UDP_REUSEADDR);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_addresss") = addr + ":" + std::to_string(port);
		return nullptr;
	}
	php::value udp_socket::recv_from(php::parameters& params)  {
		int error = uv_udp_recv_start(&socket_, alloc_cb, recv_cb);
		if(0 > error) {
			throw php::exception(uv_strerror(error), error);
		}
		socket_.data = flame::this_fiber()->push(this);
		if(params.length() >= 1) {
			addr_ = &params[0];
		}else{
			addr_ = nullptr;
		}
		if(params.length() >= 2) {
			port_ = &params[1];
		}else{
			port_ = nullptr;
		}
		return flame::async();
	}
	void udp_socket::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		udp_socket* self = f->context<udp_socket>();
		self->buffer_.rev(64 * 1024);
		buf->base = self->buffer_.data();
		buf->len  = 64 * 1024;
	}
	void udp_socket::recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(handle->data);
		udp_socket* self = f->context<udp_socket>();
		if(nread < 0) {
			f->next(php::make_exception(uv_strerror(nread), nread));
		}else if(nread == 0) {
			// again
		}else{
			self->buffer_.put(nread);
			php::string rv = std::move(self->buffer_);
			if(self->addr_ != nullptr) {
				*self->addr_ = addr2str(reinterpret_cast<const sockaddr_storage*>(addr));
			}
			if(self->port_ != nullptr) {
				*self->port_ = addrport(reinterpret_cast<const sockaddr_storage*>(addr));
			}
			uv_udp_recv_stop(&self->socket_);
			f->next(rv);
		}
	}
	php::value udp_socket::send_to(php::parameters& params) {
		php::string data = params[0];
		php::string addr = params[1];
		int         port = params[2];

		struct sockaddr_storage address;
		int error = addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// TODO 内存池管理下面对象的申请和释放？
		uv_udp_send_t* req = new uv_udp_send_t;
		req->data = flame::this_fiber()->push(this);
		uv_buf_t send {data.data(), data.length()};
		error = uv_udp_send(req, &socket_, &send, 1, reinterpret_cast<struct sockaddr*>(&address), send_cb);
		if(error < 0) {
			flame::this_fiber()->throw_exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void udp_socket::send_cb(uv_udp_send_t* req, int status) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		udp_socket* self = f->context<udp_socket>();
		delete req;
		f->next();
	}
	php::value udp_socket::close(php::parameters& params) {
		if(uv_is_closing(reinterpret_cast<uv_handle_t*>(&socket_))) return nullptr;
		socket_.data = flame::this_fiber()->push(this);
		uv_close(reinterpret_cast<uv_handle_t*>(&socket_), close_cb);
		return flame::async();
	}
	void udp_socket::close_cb(uv_handle_t* handle) {
		flame::fiber* f = reinterpret_cast<flame::fiber*>(handle->data);
		f->next();
	}
}
}
