#include "../coroutine.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_socket::unix_socket() {
		impl = new impl_t(this);
		uv_pipe_init(flame::loop, &impl->stream, 0);
	}
	unix_socket::~unix_socket() {
		impl->close(false); // 析构时 read 过程一定已经停止
	}
	php::value unix_socket::connect(php::parameters& params) {
		php::string path = params[0];
		connect_request_t* ctx = new connect_request_t {
			.co = coroutine::current,
			.obj = this,
		};
		ctx->req.data = ctx;
		uv_pipe_connect(&ctx->req, &impl->stream, path.c_str(), connect_cb);
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
	void unix_socket::after_init() {

	}
}
}
