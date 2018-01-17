#pragma once

namespace flame {
namespace net {
namespace http {
	template <class connection_t>
	class server_handler: public php::class_base {
	public:
		php::value __construct() {
			// handle_["DEFAULT"] = php::value([] (php::parameters& params) -> php::value {
			// 	php::object& res = params[1];
			// 	res->scall("writer_header", 404);
			// 	res->scall("end");
			// });
		}
		php::value __invoke(php::parameters& params) {
			if(!params[0].is_pointer()) {
				throw php::exception("misused server handler");
			}
			php::value&  ptr = params[0];
			int         type = params[1];
			uv_stream_t* svr = ptr.ptr<uv_stream_t>();

			connection_t* conn = new connection_t(this);
			if(type == UV_NAMED_PIPE) {
				uv_pipe_init(flame::loop, &conn->sock_uds, 0);
			}else if(type == UV_TCP) {
				uv_tcp_init(flame::loop, &conn->sock_tcp);
			}else{
				throw php::exception("unknown server type");
			}
			int error = uv_accept(svr, &conn->sock_);
			if(error < 0) {
				delete conn;
				return error;
			}
			conn->on_session = on_session;
			conn->start();
			return 0;
		}
		php::value put(php::parameters& params) {
			std::string path = params[0];
			handle_["PUT" + path] = params[1];
			return this;
		}
		php::value remove(php::parameters& params) {
			std::string path = params[0];
			handle_["DELETE" + path] = params[1];
			return this;
		}
		php::value post(php::parameters& params) {
			std::string path = params[0];
			handle_["POST" + path] = params[1];
			return this;
		}
		php::value get(php::parameters& params) {
			std::string path = params[0];
			handle_["GET" + path] = params[1];
			return this;
		}
		php::value handle(php::parameters& params) {
			handle_["DEFAULT"] = params[0];
			return this;
		}
	private:
		std::map<std::string, php::callable> handle_;

		static void on_session(server_connection_base* conn) {
			server_handler<connection_t>* self = reinterpret_cast<server_handler<connection_t>*>(conn->data);
			std::string path = conn->req.prop("uri");
			if(path.empty()) {
				php::fail("missing 'REQUEST_URI' in webserver config");
				uv_stop(flame::loop);
				exit(-2);
			}
			std::string method = conn->req.prop("method");
			auto fi = self->handle_.find(method + path);
			if(fi == self->handle_.end()) {
				fi = self->handle_.find("DEFAULT");
			}
			if(fi->second.is_callable()) {
				coroutine::create(fi->second, conn->req, conn->res)->start();
			}
			conn->req = nullptr;
			conn->res = nullptr;
		}
	};
}
}
}
