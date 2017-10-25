#pragma once

namespace flame {
namespace net {
	template <typename UV_TYPE_T, class MY_SERVER_T, class MY_SOCKET_T>
	class server_handler {
	public:
		UV_TYPE_T server;
		server_handler(MY_SERVER_T* svr)
		: self_(svr)
		, co_(nullptr)
		, closing_(false) {
			server.data = this;
		}
		php::value handle(php::parameters& params) {
			handle_ = params[0];
			return nullptr;
		}
		php::value run(php::parameters& params) {
			co_ = coroutine::current;
			int error = uv_listen((uv_stream_t*)&server, 1024, connection_cb);
			if(error < 0) {
				throw php::exception(uv_strerror(error), error);
			}
			ref_ = self_;
			return flame::async();
		}
		php::value close(php::parameters& params) {
			close(true);
			return flame::async();
		}
		void close(bool stop_run) {
			if(closing_) return;
			closing_ = true;
			uv_close((uv_handle_t*)&server, close_cb);
			if(stop_run && co_ != nullptr) { // 运行协程恢复
				co_->next();
				co_ = nullptr;
			}
		}
	private:
		MY_SERVER_T*    self_;
		coroutine*        co_;
		php::value       ref_;
		php::callable handle_;
		bool         closing_;
		static void connection_cb(uv_stream_t* handle, int error) {
			server_handler* self = reinterpret_cast<server_handler*>(handle->data);
			if(error < 0) {
				self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
				self->close(false);
				self->co_->fail(uv_strerror(error), error);
				return;
			}
			php::object  cli = php::object::create<MY_SOCKET_T>();
			MY_SOCKET_T* cpp = cli.native<MY_SOCKET_T>();
			error = uv_accept(handle, (uv_stream_t*)&cpp->handler->socket);
			if(error < 0) {
				self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用3
				self->co_->fail(uv_strerror(error), error);
				self->close(false);
				return;
			}
			cpp->after_init();
			coroutine::start(self->handle_, cli);
		}
		static void close_cb(uv_handle_t* handle) {
			delete reinterpret_cast<
				server_handler<UV_TYPE_T, MY_SERVER_T, MY_SOCKET_T>*
			>(handle->data);
		}
	};
}
}
