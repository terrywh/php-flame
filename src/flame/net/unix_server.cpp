#include "../fiber.h"
#include "../process_manager.h"
#include "unix_server.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_server::unix_server()
	: stream_server(reinterpret_cast<uv_stream_t*>(&server_)) {

	}
	php::value unix_server::handle(php::parameters& params) {
		handle_ = params[0];
		return this;
	}
	php::value unix_server::bind(php::parameters& params) {
		php::string path = params[0];
		if(path.c_str()[0] != '/') {
			throw php::exception("bind failed: only absolute path is allowed");
		}
		prop("local_address") = path;
		uv_pipe_init(flame::loop, &server_, 0);
		if(flame::process_type == PROCESS_MASTER) {
			unlink(path.c_str());
			int error = uv_pipe_bind(&server_, path.c_str());
			if(error < 0) {
				throw php::exception(uv_strerror(error), error);
			}
		}
		return nullptr;
	}
	int unix_server::accept(uv_stream_t* server) {
		php::object  sobj  = php::object::create<unix_socket>();
		unix_socket* pobj  = sobj.native<unix_socket>();
		uv_pipe_init(flame::loop, &pobj->socket_, 0);
		int error = uv_accept(server, reinterpret_cast<uv_stream_t*>(&pobj->socket_));
		if(error < 0) return error;
		fiber::start(handle_, sobj);
		return 0;
	}
}
}
