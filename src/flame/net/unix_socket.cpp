#include "../coroutine.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_socket::unix_socket()
	: handler_(this) {
		uv_pipe_init(flame::loop, &handler_.socket, 0);
	}
	typedef struct connect_request_t {
		coroutine*   co;
		// unix_server* us;
		php::value   rf;
		uv_connect_t uv;
	} connect_request_t;
	php::value unix_socket::connect(php::parameters& params) {
		php::string path = params[0];
		connect_request_t* ctx = new connect_request_t {
			.co = coroutine::current,
			// .us = this,
			.rf = this,
		};
		ctx->uv.data = ctx;
		uv_pipe_connect(&ctx->uv, &handler_.socket, path.c_str(), connect_cb);
		prop("remote_address") = path;
		return flame::async();
	}
	void unix_socket::connect_cb(uv_connect_t* handle, int error) {
		connect_request_t* ctx = static_cast<connect_request_t*>(handle->data);
		if(error < 0) {
			ctx->co->fail(uv_strerror(error), error);
		}else{
			ctx->co->next();
		}
		delete ctx;
	}
}
}
