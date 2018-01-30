#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "net.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_socket::unix_socket()
	: sck((uv_pipe_t*)malloc(sizeof(uv_pipe_t)))
	, rdr((uv_stream_t*)sck)
	, wtr((uv_stream_t*)sck)
	// 默认可读可写（某些特殊的 PIPE 可能只读或只写）
	, flags(CAN_READ | CAN_WRITE) {
		uv_pipe_init(flame::loop, sck, 0);
	}
	unix_socket::~unix_socket() {
		close();
	}
	typedef struct connect_request_t {
		coroutine*     co;
		uv_connect_t  req;
	} connect_request_t;
	php::value unix_socket::connect(php::parameters& params) {
		if(!(flags & CAN_WRITE)) throw php::exception("connect failed: current unix_socket is read only");
		
		php::string path = params[0];
		connect_request_t* ctx = new connect_request_t { coroutine::current };
		ctx->req.data = ctx;
		uv_pipe_connect(&ctx->req, sck, path.c_str(), connect_cb);
		return flame::async(this);
	}
	void unix_socket::connect_cb(uv_connect_t* handle, int error) {
		connect_request_t* ctx = static_cast<connect_request_t*>(handle->data);
		if(error < 0) ctx->co->fail(uv_strerror(error), error);
		else ctx->co->next();
		delete ctx;
	}
	php::value unix_socket::read(php::parameters& params) {
		if(!(flags & CAN_READ)) throw php::exception("failed to read: current unix_socket is write only");
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
	php::value unix_socket::read_all(php::parameters& params) {
		if(sck == nullptr) throw php::exception("failed to read: socket is already closed");
		
		rdr.read_all();
		return flame::async(this);
	}
	php::value unix_socket::write(php::parameters& params) {
		if(!(flags & CAN_WRITE)) throw php::exception("failed to write: current unix_socket is read only");
		if(sck == nullptr) throw php::exception("failed to write: socket is already closed");
		
		php::string str = params[0].to_string();
		wtr.write(str);
		return flame::async(this);
	}
	php::value unix_socket::close(php::parameters& params) {
		close();
		return nullptr;
	}
	// property local_address ""
	// property remote_address ""
	void unix_socket::close(int err) {
		if(sck) {
			rdr.close();
			wtr.close();
			uv_close((uv_handle_t*)sck, free_handle_cb);
			sck = nullptr;
		}
	}
	void unix_socket::after_init() {
		// 为使流程与 tcp_socket 一致，保留此函数
	}
}
}
