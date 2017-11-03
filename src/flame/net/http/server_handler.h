#pragma once

namespace flame {
namespace net {
namespace http {
	class server_handler_base: public php::class_base {
	public:
		virtual void on_request(php::object& req, php::object& res) = 0;
	};
	template <class connection_t>
	class server_handler: public server_handler_base {
	public:
		php::value __invoke(php::parameters& params) {
			if(!params[0].is_pointer()) {
				throw php::exception("misused server handler");
			}
			php::value&  ptr = params[0];
			uv_stream_t* svr = (uv_stream_t*)Z_PTR((zval&)ptr);

			connection_t* pobj = new connection_t(this);
			if(svr->type == UV_NAMED_PIPE) {
				uv_pipe_init(flame::loop, &pobj->socket_pipe, 0);
			}else if(svr->type == UV_TCP) {
				uv_tcp_init(flame::loop, &pobj->socket_tcp);
			}
			int error = uv_accept(svr, &pobj->socket);
			if(error < 0) {
				delete pobj;
				return error;
			}
			pobj->start();
			return 0;
		}
		php::value put(php::parameters& params) {
			std::string path = params[0];
			handle_put[path] = params[1];
			return this;
		}
		php::value remove(php::parameters& params) {
			std::string path = params[0];
			handle_delete[path] = params[1];
			return this;
		}
		php::value post(php::parameters& params) {
			std::string path = params[0];
			handle_post[path] = params[1];
			return this;
		}
		php::value get(php::parameters& params) {
			std::string path = params[0];
			handle_get[path] = params[1];
			return this;
		}
		php::value handle(php::parameters& params) {
			handle_def = params[0];
			return this;
		}
		void on_request(php::object& req, php::object& res) {
			php::string& path   = req.prop("uri");
			php::string& method = req.prop("method");

			if(path.is_empty()) {
				php::fail("missing 'REQUEST_URI' in webserver config");
				uv_stop(flame::loop);
				exit(-2);
			}
			std::map<std::string, php::callable>* map;
			if(std::strncmp(method.data(), "PUT", 3) == 0) {
				map = &handle_put;
			}else if(std::strncmp(method.data(), "DELETE", 7) == 0) {
				map = &handle_delete;
			}else if(std::strncmp(method.data(), "POST", 4) == 0) {
				map = &handle_post;
			}else if(std::strncmp(method.data(), "GET", 3) == 0) {
				map = &handle_get;
			}else{
				coroutine::start(handle_def, req, res);
				return;
			}
			auto fi = map->find(path);
			if(fi != map->end()) {
				coroutine::start(fi->second, req, res);
			}else{
				coroutine::start(handle_def, req, res);
			}
		}
	private:
		std::map<std::string, php::callable> handle_put;
		std::map<std::string, php::callable> handle_delete;
		std::map<std::string, php::callable> handle_post;
		std::map<std::string, php::callable> handle_get;
		php::callable                        handle_def;
	};
}
}
}
