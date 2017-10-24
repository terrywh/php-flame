#include "../coroutine.h"
#include "../../util/sock.h"
#include "net.h"
#include "udp_socket.h"

namespace flame {
namespace net {
	udp_socket::udp_socket() {
		uv_udp_init(flame::loop, &socket_);
		socket_.data = this;
	}
	php::value udp_socket::bind(php::parameters& params) {
		std::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int error = util::sock_addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// uv_udp_init_ex 会创建 socket
		uv_udp_init_ex(flame::loop, &socket_, address.ss_family);
		// 然后才能进行 SO_REUSEPORT 设置
		util::sock_reuseport(reinterpret_cast<uv_handle_t*>(&socket_));
		error = uv_udp_bind(&socket_, reinterpret_cast<struct sockaddr*>(&address), UV_UDP_REUSEADDR);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_addresss") = addr + ":" + std::to_string(port);
		return nullptr;
	}
	php::value udp_socket::recv(php::parameters& params)  {
		int error = uv_udp_recv_start(&socket_, alloc_cb, recv_cb);
		if(0 > error) {
			throw php::exception(uv_strerror(error), error);
		}
		refer_   = this; // 保留对象引用，防止异步过程丢失
		routine_ = coroutine::current;
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
		udp_socket* self = static_cast<udp_socket*>(handle->data);
		self->rbuffer_ = php::string(64 * 1024);
		buf->base = self->rbuffer_.data();
		buf->len  = 64 * 1024;
	}
	void udp_socket::recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
		udp_socket* self = static_cast<udp_socket*>(handle->data);
		if(nread < 0) {
			self->routine_->fail(uv_strerror(nread), nread);
			self->refer_ = nullptr;
		}else if(nread == 0) {
			// again
		}else{
			self->rbuffer_.length() = nread;
			if(self->addr_ != nullptr) {
				*self->addr_ = util::sock_addr2str(reinterpret_cast<const sockaddr_storage*>(addr));
			}
			if(self->port_ != nullptr) {
				*self->port_ = util::sock_addrport(reinterpret_cast<const sockaddr_storage*>(addr));
			}
			uv_udp_recv_stop(&self->socket_);
			self->routine_->next(std::move(self->rbuffer_));
			self->refer_ = nullptr;
		}
	}

	typedef struct send_request_t {
		flame::coroutine* co;
		php::string       rs; // 保留数据引用防止丢失
		php::value        ro; // 保留对象阴影防止丢失
		uv_udp_send_t     uv;
	} send_request_t;

	php::value udp_socket::send(php::parameters& params) {
		php::string addr = params[1];
		int         port = params[2];

		struct sockaddr_storage address;
		int error = util::sock_addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// TODO 内存池管理下面对象的申请和释放？
		send_request_t* req = new send_request_t {
			.co = coroutine::current,
			.rs = params[0],
			.ro = this,
		};
		req->uv.data = req;
		uv_buf_t data { req->rs.data(), req->rs.length() };
		error = uv_udp_send(&req->uv, &socket_, &data, 1, reinterpret_cast<struct sockaddr*>(&address), send_cb);
		if(error < 0) {
			delete req;
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void udp_socket::send_cb(uv_udp_send_t* req, int status) {
		auto ctx = static_cast<send_request_t*>(req->data);
		ctx->co->next();
		delete ctx;
	}
	php::value udp_socket::close(php::parameters& params) {
		if(!uv_is_closing(reinterpret_cast<uv_handle_t*>(&socket_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(&socket_), nullptr);
		}
		return nullptr;
	}
}
}
