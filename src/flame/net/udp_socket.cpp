#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "net.h"
#include "udp_socket.h"
#include "udp_packet.h"

namespace flame {
namespace net {
	udp_socket::udp_socket()
	: co_(nullptr)
	, init_(false)
	, bound_(false) {
		sck = (uv_udp_t*)malloc(sizeof(uv_udp_t));
		sck->data = this;
	}
	udp_socket::~udp_socket() {
		close(); // 对象析构，读取过程一定已经停止
	}
	php::value udp_socket::bind(php::parameters& params) {
		if(sck == nullptr) throw php::exception("bind failed: socket is already closed");
		
		std::string addr = params[0];
		int         port = params[1].to_long(), err;
		struct sockaddr_storage address;
		err = sock_addrfrom(&address, addr.c_str(), port);
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		uv_udp_init_ex(flame::loop, sck, address.ss_family);
		init_ = true;
		// 然后才能进行 SO_REUSEPORT 设置
		sock_reuseport((uv_handle_t*)sck);
		err = uv_udp_bind(sck, (struct sockaddr*)&address, UV_UDP_REUSEADDR);
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		// 服务器属性
		prop("local_addresss", 14) = addr;
		prop("local_port", 10) = port;
		bound_ = true;
		return nullptr;
	}
	php::value udp_socket::recv(php::parameters& params)  {
		if(sck == nullptr) throw php::exception("recv failed: socket is already closed"); // 已关闭
		if(!bound_)  throw php::exception("recv failed: socket is not bound");
		
		int err = uv_udp_recv_start(sck, alloc_cb, recv_cb);
		if(0 > err) {
			throw php::exception(uv_strerror(err), err);
		}
		co_ = coroutine::current;
		return flame::async(this);
	}
	void udp_socket::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		udp_socket* self = static_cast<udp_socket*>(handle->data);
		buf->base = self->buffer_;
		buf->len  = sizeof(self->buffer_);
	}
	void udp_socket::recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
		udp_socket* self = static_cast<udp_socket*>(handle->data);
		if(nread == UV_EOF) {
			self->close();
		}else if(nread < 0) {
			self->close(nread);
		}else if(nread == 0) {
			// again
		}else{
			uv_udp_recv_stop(self->sck);
			// 构造 udp_packet 并返回
			php::object obj = php::object::create<udp_packet>();
			udp_packet* cpp = obj.native<udp_packet>();
			// 策略上使用额外的小块申请 + 拷贝代替不断的申请 64k 大块内存
			cpp->init( php::string(self->buffer_, nread), reinterpret_cast<const struct sockaddr_storage*>(addr));
			// 由于 co->next() 可能包含下一次的 recv()
			// 故，self->co_ = nullptr 必须在 co->next() 之前
			// 否则可能导致误将下一次的 co_ 清理
			coroutine* co = self->co_;
			self->co_ = nullptr;
			co->next( std::move(obj) );
		}
	}
	typedef struct send_request_t {
		flame::coroutine* co;
		udp_socket*     self;
		php::string     data; // 保留数据引用防止丢失
		uv_udp_send_t    req;
	} send_request_t;
	php::value udp_socket::send(php::parameters& params) {
		if(sck == nullptr) throw php::exception("send failed: socket is already closed"); // 已关闭
		
		php::string addr = params[1];
		int         port = params[2].to_long(), err;

		struct sockaddr_storage address;
		err = sock_addrfrom(&address, addr.c_str(), port);
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		if(!init_) {
			uv_udp_init_ex(flame::loop, sck, address.ss_family);
			init_ = true; // 初始化后释放逻辑不同
		}
		send_request_t* ctx = new send_request_t {
			coroutine::current, this, params[0].to_string()
		};
		ctx->req.data = ctx;
		uv_buf_t data { ctx->data.data(), ctx->data.length() };
		err = uv_udp_send(&ctx->req, sck, &data, 1, reinterpret_cast<struct sockaddr*>(&address), send_cb);
		if(err < 0) {
			delete ctx;
			throw php::exception(uv_strerror(err), err);
		}
		if(!bound_) {
			int len = sizeof(struct sockaddr_storage);
			uv_udp_getsockname(sck, (struct sockaddr*)&address, &len);
			
			prop("local_address") = sock_addr2str(&address);
			prop("local_port") = sock_addrport(&address);
		}
		return flame::async(this);
	}
	void udp_socket::send_cb(uv_udp_send_t* handle, int status) {
		auto ctx = reinterpret_cast<send_request_t*>(handle->data);
		if(status == UV_ECANCELED) {
			ctx->co->next();
		}else if(status < 0) {
			ctx->self->close(status);
		}else{
			ctx->co->next();
		}
		delete ctx;
	}
	void udp_socket::close(int err) {
		if(co_) {
			coroutine* co = co_;
			if(err == 0) co->next();
			else co->fail(uv_strerror(err), err);
		}
		if(sck) {
			if(init_) {
				uv_udp_recv_stop(sck);
				uv_close((uv_handle_t*)sck, free_handle_cb);
			}
			else free(sck); // 未初始化，直接释放即可
			sck = nullptr;
		}
	}
	php::value udp_socket::close(php::parameters& params) {
		close();
		return nullptr;
	}
}
}
