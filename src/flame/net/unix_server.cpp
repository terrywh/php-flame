#include "../coroutine.h"
#include "unix_server.h"
#include "unix_socket.h"

namespace flame {
namespace net {
	unix_server::unix_server() {
		server_.data = this;
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
		int error = uv_pipe_bind(&server_, path.c_str());
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return nullptr;
	}
	php::value unix_server::run(php::parameters& params) {
		co_ = coroutine::current;
		int error = uv_listen(reinterpret_cast<uv_stream_t*>(&server_), 1024, connection_cb);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void unix_server::connection_cb(uv_stream_t* handle, int error) {
		unix_server* self = reinterpret_cast<unix_server*>(handle->data);
		if(error < 0) {
			self->co_->fail(uv_strerror(error), error);
			return;
		}
		php::object  cli = php::object::create<unix_socket>();
		unix_socket* cpp = cli.native<unix_socket>();
		error = uv_accept(handle, (uv_stream_t*)&cpp->handler_.socket);
		if(error < 0) {
			self->co_->fail(uv_strerror(error), error);
			return;
		}
		cli.prop("remote_address") = self->prop("local_address");
		coroutine::start(self->handle_, std::move(cli));
	}
}
}
