#pragma once

namespace flame {
namespace net {
namespace http {
	template <class connection_t>
	class server_handler: public php::class_base {
	public:
		php::value __construct() {
			// 默认的 HANDLER 必须存在，但什么都不做
			handle_default = php::value([] (php::parameters& params) -> php::value {
				return nullptr;
			});
			return nullptr;
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
			return nullptr;
		}
		php::value put(php::parameters& params) {
			set_handler("PUT", params[0], params[1]);
			return php::object(this);
		}
		php::value remove(php::parameters& params) {
			set_handler("DELETE", params[0], params[1]);
			return php::object(this);
		}
		php::value post(php::parameters& params) {
			set_handler("POST", params[0], params[1]);
			return php::object(this);
		}
		php::value get(php::parameters& params) {
			set_handler("GET", params[0], params[1]);
			return php::object(this);
		}
		php::value handle(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			handle_default = params[0];
			return php::object(this);
		}
		php::value before(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			handle_before = params[0];
			return php::object(this);
		}
		php::value after(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			handle_after = params[0];
			return php::object(this);
		}
	private:
		void set_handler(const std::string& method, const std::string& path, const php::callable& cb) {
			if(!cb.is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			handle_[ method + path ] = cb;
		}

		std::map<std::string, php::callable> handle_;
		php::callable                        handle_before;
		php::callable                        handle_after;
		php::callable                        handle_default;
		typedef struct session_context_t {
			php::object  req;
			php::object  res;
			php::callable cb;
			server_handler<connection_t>* self;
		} session_context_t;

		static void on_session_before(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			if(ctx->self->handle_before.is_callable()) {
				coroutine::create(ctx->self->handle_before, {
					ctx->req, ctx->res,
					(ctx->cb == ctx->self->handle_default) ? php::BOOL_FALSE : php::BOOL_TRUE,
				})
				->after(on_session_middle, ctx)
				->start();
			}else{
				on_session_middle(ctx);
			}
		}
		static void on_session_middle(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			coroutine::create(ctx->cb, {ctx->req, ctx->res})
				->after(on_session_after, ctx)
				->start();
		}
		static void on_session_after(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			if(ctx->self->handle_after.is_callable()) {
				coroutine::create(ctx->self->handle_after, {
					ctx->req, ctx->res,
					(ctx->cb == ctx->self->handle_default) ? php::BOOL_FALSE : php::BOOL_TRUE,
				})
				->after(on_session_finish, ctx)
				->start();
			}else{
				on_session_finish(ctx);
			}
		}
		static void on_session_finish(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			delete ctx;
		}
		inline static void on_session(server_connection_base* conn) {
			server_handler<connection_t>* self = reinterpret_cast<server_handler<connection_t>*>(conn->data);

			std::string   path = conn->req.prop("uri");
			std::string method = conn->req.prop("method");
			session_context_t* ctx = new session_context_t {
				// 使用 move 形式对象构造解除 conn 内部对 req / res 的引用
				std::move(conn->req), std::move(conn->res), self->handle_default, self,
			};
			auto fi = self->handle_.find(method + path);
			if(fi != self->handle_.end()) {
				ctx->cb = fi->second;
			}
			
			on_session_before(ctx);
		}
	};
}
}
}
