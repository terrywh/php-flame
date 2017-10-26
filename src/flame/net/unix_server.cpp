#include "../coroutine.h"
#include "unix_socket.h"
#include "unix_server.h"

namespace flame {
namespace net {
	unix_server::unix_server() {
		handler = new handler_t(this);
		uv_pipe_init(flame::loop, &handler->server, 0);
	}
	unix_server::~unix_server() {
		handler->close(false); // 析构时 run 过程一定已经停止
	}
	php::value unix_server::bind(php::parameters& params) {
		php::string path = params[0];
		if(path.c_str()[0] != '/') {
			throw php::exception("bind failed: only absolute path is allowed");
		}
		prop("local_address") = path;
		int error = uv_pipe_bind(&handler->server, path.c_str());
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return nullptr;
	}
}
}
