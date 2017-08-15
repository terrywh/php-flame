#include "../../fiber.h"
#include "server.h"
#include "server_connection.h"
#include "server_response.h"

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
		return this;
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
	php::value server::run(php::parameters& params) {
		// 若未指定，提供默认的 handle
		if(handle_def_.is_empty()) {
			handle_def_ = php::value([this] (php::parameters& params) -> php::value {
				php::object& obj = params[1];
				flame::this_fiber()->push([obj] (php::value& rv) {
					// 这里没有什么要做的，这里仅用来保存 obj 的引用
				});
				server_response* res = obj.native<server_response>();
				obj.prop("status")      = 404;
				obj.prop("header_sent") = true;
				obj.prop("ended")       = true;
				res->buffer_head();
				res->buffer_body("router not found", 16);
				res->buffer_ending();
				res->buffer_write();
				return flame::async();
			});
		}
		return stream_server::run(params);
	}

	void server::accept(uv_stream_t* s) {
		server_connection* conn = reinterpret_cast<server_connection*>(s->data);
		conn->start();
		// conn->delref();
	}
	uv_stream_t* server::create_stream() {
		// php::object sobj = php::object::create<server_connection>();
		// server_connection* pobj = sobj.native<server_connection>();
		server_connection* pobj = new server_connection();
		pobj->server_ = this;
		pobj->socket_.data = pobj;
		uv_pipe_init(flame::loop, &pobj->socket_, 0);
		// pobj->addref();
		return reinterpret_cast<uv_stream_t*>(&pobj->socket_);
	}
	void server::on_request(server_connection* conn, php::object&& req) {
		php::object      res = php::object::create<server_response>();
		server_response* obj = res.native<server_response>();
		obj->conn_ = conn;
		php::string& path = req.prop("uri");
		if(path.is_empty()) {
			php::fail("missing 'REQUEST_URI' in webserver config");
			uv_stop(flame::loop);
			exit(-2);
		}else{
			auto hi = handle_map_.find(path);
			if(hi != handle_map_.end()) {
				fiber::start(hi->second, req, res);
			}else{
				fiber::start(handle_def_, req, res);
			}
		}
	}
}
}
}
