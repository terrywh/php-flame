#include "../flame.h"
#include "../coroutine.h"
#include "../../util/sock.h"
#include "net.h"
#include "udp_socket.h"

namespace flame {
namespace net {
	udp_socket::udp_socket()
	: cor_(nullptr) {
		stream = (uv_udp_t*)malloc(sizeof(uv_udp_t));
		uv_udp_init(flame::loop, stream);
		stream->data = this;
	}
	udp_socket::~udp_socket() {
		close(false); // 对象析构，读取过程一定已经停止
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
		uv_udp_init_ex(flame::loop, stream, address.ss_family);
		// 然后才能进行 SO_REUSEPORT 设置
		util::sock_reuseport((uv_handle_t*)stream);
		error = uv_udp_bind(stream, (struct sockaddr*)&address, UV_UDP_REUSEADDR);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_addresss") = addr + ":" + std::to_string(port);
		return nullptr;
	}
	php::value udp_socket::recv(php::parameters& params)  {
		if(stream == nullptr) return nullptr; // 已关闭
		int error = uv_udp_recv_start(stream, alloc_cb, recv_cb);
		if(0 > error) {
			throw php::exception(uv_strerror(error), error);
		}
		ref_ = this; // 保留对象引用，防止异步过程丢失
		cor_ = coroutine::current;
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
		if(nread == UV_EOF) {
			self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
			self->close(true);
		}else if(nread < 0) {
			self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
			self->close(false);
			self->cor_->fail(uv_strerror(nread), nread);
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
			uv_udp_recv_stop(self->stream);
			self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
			self->cor_->next(std::move(self->rbuffer_));
		}
	}

	typedef struct send_request_t {
		flame::coroutine* co;
		udp_socket*       us;
		php::string       rs; // 保留数据引用防止丢失
		php::value        ro; // 保留对象阴影防止丢失
		uv_udp_send_t     uv;
	} send_request_t;

	php::value udp_socket::send(php::parameters& params) {
		if(stream == nullptr) throw php::exception("socket is already closed"); // 已关闭

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
			.us = this,
			.rs = params[0],
			.ro = this,
		};
		req->uv.data = req;
		uv_buf_t data { req->rs.data(), req->rs.length() };
		error = uv_udp_send(&req->uv, stream, &data, 1, reinterpret_cast<struct sockaddr*>(&address), send_cb);
		if(error < 0) {
			delete req;
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void udp_socket::send_cb(uv_udp_send_t* handle, int status) {
		auto ctx = reinterpret_cast<send_request_t*>(handle->data);
		if(status == UV_ECANCELED) {
			ctx->co->next();
		}else if(status < 0) {
			ctx->co->fail(uv_strerror(status), status);
			ctx->us->close(true);
		}else{
			ctx->co->next();
		}
		delete ctx;
	}
	void udp_socket::close(bool stop_recv) {
		if(stream == nullptr) return;
		if(stop_recv && cor_ != nullptr) { // 读取协程继续
			cor_->next();
		}
		uv_close((uv_handle_t*)socket, free_handle_cb);
		stream = nullptr;
	}
	php::value udp_socket::close(php::parameters& params) {
		close(true);
		return nullptr;
	}
}
}
