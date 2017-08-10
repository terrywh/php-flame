#include "../fiber.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_socket::unix_socket()
	: stream_socket(reinterpret_cast<uv_stream_t*>(&socket_)) {
		uv_pipe_init(flame::loop, &socket_, 0);
		// socket_.data = this;
	}

	php::value unix_socket::connect(php::parameters& params) {
		php::string path = params[0];
		uv_connect_t* req = new uv_connect_t;
		req->data = flame::this_fiber()->push(this);
		uv_pipe_connect(req, &socket_, path.c_str(), connect_cb);
		prop("remote_address") = path;
		return flame::async();
	}
	void unix_socket::connect_cb(uv_connect_t* req, int error) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		unix_socket* self = f->context<unix_socket>();
		delete req;
		
		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
		}else{
			f->next();
		}
	}
}
}