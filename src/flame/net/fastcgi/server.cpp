#include "server.h"
#include "connection.h"

namespace flame {
namespace net {
namespace fastcgi {
	server::server()
	: stream_server(reinterpret_cast<uv_stream_t*>(&server_)) {

	}
	php::value server::handle(php::parameters& params) {
		if(params.length() >= 2) {
			std::string path  = params[0];
			if(path.length() > 0) {
				handle_map_[path] = params[1];
			}
		}else if (params.length() >= 1) {
			handle_def_ = params[0];
		}else{
			throw php::exception("only Generator function can be use as handler");
		}		
		return nullptr;
	}
	php::value server::bind(php::parameters& params) {
		php::string& path = params[0];
		uv_pipe_init(flame::loop, &server_, 0);
		int error = uv_pipe_bind(&server_, path.c_str());
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_address") = path;
		return nullptr;
	}
	void server::accept(uv_stream_t* s) {
		connection* conn = reinterpret_cast<connection*>(s->data);
		conn->start();
		// conn->delref();
	}
	uv_stream_t* server::create_stream() {
		// php::object sobj = php::object::create<connection>();
		// connection* pobj = sobj.native<connection>();
		connection* pobj = new connection();
		pobj->socket_.data = pobj;
		uv_pipe_init(flame::loop, &pobj->socket_, 0);
		// pobj->addref();
		return reinterpret_cast<uv_stream_t*>(&pobj->socket_);
	}
}
}
}
