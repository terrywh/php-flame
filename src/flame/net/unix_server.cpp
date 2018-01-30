#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "net.h"
#include "unix_socket.h"
#include "unix_server.h"

namespace flame {
namespace net {
	unix_server::unix_server()
	: svr((uv_pipe_t*)malloc(sizeof(uv_pipe_t)))
	, acc((uv_stream_t*)svr)
	, bound_(false) {
		uv_pipe_init(flame::loop, svr, 0);
	}
	unix_server::~unix_server() {
		close();
	}
	php::value unix_server::handle(php::parameters& params) {
		acc.handle(params[0]);
		return nullptr;
	}
	php::value unix_server::bind(php::parameters& params) {
		if(bound_) throw php::exception("failed to bind: server is already bound");
		
		php::string path = params[0].to_string();
		if(path.c_str()[0] != '/') {
			throw php::exception("failed to bind: only absolute path is allowed");
		}
	
		int err = uv_pipe_bind(svr, path.c_str());
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		bound_ = true;
		return nullptr;
	}
	php::value unix_server::run(php::parameters& params) {
		if(!bound_) throw php::exception("failed to run: server is not bound");
		
		acc.run();
		return flame::async(this);
	}
	php::value unix_server::close(php::parameters& params) {
		close();
		return nullptr;
	}
	void unix_server::close() {
		if(svr) {
			acc.close();
			uv_close((uv_handle_t*)svr, flame::free_handle_cb);
			svr = nullptr;
		}
	}
}
}
