#pragma once

namespace flame {
namespace net {
namespace http {
	template <class connection_t>
	class server_handler: public php::class_base {
	public:
		php::value __construct() {
			// 默认的 HANDLER 必须存在，但什么都不做
			default_ = php::value([] (php::parameters& params) -> php::value {
				return nullptr;
			});
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
			set_handler("PUT", params[0], params[1]);
			return this;
		}
		php::value remove(php::parameters& params) {
			set_handler("DELETE", params[0], params[1]);
			return this;
		}
		php::value post(php::parameters& params) {
			set_handler("POST", params[0], params[1]);
			return this;
		}
		php::value get(php::parameters& params) {
			set_handler("GET", params[0], params[1]);
			return this;
		}
		php::value handle(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			default_ = params[0];
			return this;
		}
		php::value before(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			before_ = params[0];
			return this;
		}
		php::value after(php::parameters& params) {
			if(!params[0].is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			after_ = params[0];
			return this;
		}
	private:
		void set_handler(const std::string& method, const std::string& path, const php::callable& cb) {
			if(!cb.is_callable()) {
				throw php::exception("only callable can be used as handler");
			}
			handle_[ method + path ] = cb;
		}

		std::map<std::string, php::callable> handle_;
		php::callable                        before_;
		php::callable                         after_;
		php::callable                       default_;
		typedef struct session_context_t {
			php::object req;
			php::object res;
			server_handler<connection_t>* self;
		} session_context_t;

		static void on_session_before(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			if(ctx->self->before_.is_callable()) {
				coroutine* co = coroutine::create(ctx->self->before_, ctx->req, ctx->res);
				if(co != nullptr) {
					co->after(on_session_handle, ctx)->start();
					return;
				}
			}
			on_session_handle(ctx);
		}
		static void on_session_handle(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			std::string      path = ctx->req.prop("uri");
			std::string    method = ctx->req.prop("method");
			php::callable& handle = ctx->self->default_;
			auto fi = ctx->self->handle_.find(method + path);
			if(fi != ctx->self->handle_.end()) {
				handle = fi->second;
			}
			coroutine* co = coroutine::create(handle, ctx->req, ctx->res);
			if(co == nullptr) {
				on_session_after(ctx);
			}else{
				co->after(on_session_after, ctx)->start();
			}
		}
		static void on_session_after(void* data) {
			session_context_t* ctx = reinterpret_cast<session_context_t*>(data);
			if(ctx->self->after_.is_callable()) {
				coroutine::start(ctx->self->after_, ctx->req, ctx->res);
			}
			delete ctx;
		}
		inline static void on_session(server_connection_base* conn) {
			session_context_t* ctx = new session_context_t {
				std::move(conn->req), // 使用 move 形式对象构造解除 conn 内部的引用
				std::move(conn->res),
				reinterpret_cast<server_handler<connection_t>*>(conn->data)
			};
			on_session_before(ctx);
		}
	};
}
}
}
