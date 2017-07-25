#include "../fiber.h"
#include "unix_server.h"

namespace flame {
namespace net {
	unix_server::unix_server()
	: stream_server(reinterpret_cast<uv_stream_t*>(&server_)) {

	}
	php::value unix_server::handle(php::parameters& params) {
		handle_ = params[0];
		++status_;
		return nullptr;
	}
	php::value unix_server::bind(php::parameters& params) {
		php::string& path = params[0];
		uv_pipe_init(flame::loop, &server_, 0);
		int error = uv_pipe_bind(&server_, path.c_str());
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		++status_;
		// 服务器属性
		prop("local_address") = path;
		return nullptr;
	}
	void unix_server::accept(uv_stream_t* s) {
		unix_socket* sock = reinterpret_cast<unix_socket*>(s->data);
		fiber::start(handle_, sock);
	}
	uv_stream_t* unix_server::create_stream() {
		php::object  sobj  = php::object::create<unix_socket>();
		unix_socket* pobj  = sobj.native<unix_socket>();
		pobj->socket_.data = pobj;
		uv_pipe_init(flame::loop, &pobj->socket_, 0);
		return reinterpret_cast<uv_stream_t*>(&pobj->socket_);
	}
}
}
