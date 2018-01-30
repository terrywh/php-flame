#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "net.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket()
	: sck((uv_tcp_t*)malloc(sizeof(uv_tcp_t)))
	, rdr((uv_stream_t*)sck)
	, wtr((uv_stream_t*)sck) {
		uv_tcp_init(flame::loop, sck);
	}
	tcp_socket::~tcp_socket() {
		close();
	}
	typedef struct connect_request_t {
		coroutine*     co;
		tcp_socket*  self;
		uv_connect_t  req;
	} connect_request_t;
	php::value tcp_socket::connect(php::parameters& params) {
		php::string addr = params[0];
		int         port = params[1].to_long(), err;
		struct sockaddr_storage address;
		err = sock_addrfrom(&address, addr.c_str(), port);
		if(err != 0) {
			throw php::exception(uv_strerror(err), err);
		}
		connect_request_t* ctx = new connect_request_t {
			coroutine::current, this
		};
		ctx->req.data = ctx;
		err = uv_tcp_connect(&ctx->req, sck, reinterpret_cast<struct sockaddr*>(&address), connect_cb);
		if(err != 0) {
			delete ctx;
			throw php::exception(uv_strerror(err), err);
		}
		return flame::async(this);
	}
	void tcp_socket::connect_cb(uv_connect_t* handle, int err) {
		connect_request_t* ctx = static_cast<connect_request_t*>(handle->data);
		if(err < 0) {
			ctx->co->fail(uv_strerror(err), err);
		}else{
			ctx->self->after_init();
			ctx->co->next();
		}
		delete ctx;
	}
	php::value tcp_socket::read(php::parameters& params) {
		if(sck == nullptr) throw php::exception("failed to read: socket is already closed");
		
		if(params.length() > 0) {
			if(params[0].is_long()) {
				size_t size = params[0];
				if(size <= 0) throw php::exception("failed to read: length must be >= 0");
				rdr.read(size);
			}else if(params[0].is_string()) {
				php::string& endl = params[0];
				if(endl.length() <= 0) throw php::exception("failed to read: endl must containe at least one character");
				rdr.read(endl);
			}else{
				throw php::exception("failed to read: unsupported read method");
			}
		}else{
			rdr.read();
		}
		return flame::async(this);
	}
	php::value tcp_socket::read_all(php::parameters& params) {
		if(sck == nullptr)  throw php::exception("failed to read: socket is already closed");
		
		rdr.read_all();
		return flame::async(this);
	}
	php::value tcp_socket::write(php::parameters& params) {
		if(sck == nullptr) throw php::exception("failed to write: socket is already closed");
		
		php::string str = params[0].to_string();
		wtr.write(str);
		return flame::async(this);
	}
	php::value tcp_socket::close(php::parameters& params) {
		close();
		return nullptr;
	}
	// property local_address ""
	// property remote_address ""
	void tcp_socket::close() {
		if(sck) {
			rdr.close();
			wtr.close();
			uv_close((uv_handle_t*)sck, free_handle_cb);
			sck = nullptr;
		}
	}
	void tcp_socket::after_init() {
		struct sockaddr_storage address;
		int                     addrlen = sizeof(address);
		uv_tcp_getsockname(sck, reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("local_address") = sock_addr2str(&address);
		prop("local_port") = sock_addrport(&address);
			
		uv_tcp_getpeername(sck, reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("remote_address") = sock_addr2str(&address);
		prop("remote_port") = sock_addrport(&address);
	}
}
}
