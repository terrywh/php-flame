#pragma once

namespace flame {
namespace net {
	template <typename UV_TYPE_T, class MY_SERVER_T, class MY_SOCKET_T>
	class server_impl {
	public:
		typedef server_impl<UV_TYPE_T, MY_SERVER_T, MY_SOCKET_T> impl_t;
		UV_TYPE_T server;
		server_impl(MY_SERVER_T* svr)
		: self_(svr)
		, co_(nullptr)
		, cb_type(0)
		, closing_(false) {
			server.data = this;
		}
		php::value handle(php::parameters& params) {
			if(params[0].is_callable()) {
				cb_ = params[0];
				// 限定特殊类型标志
				if(cb_.is_object() && !cb_.is_closure()
					&& static_cast<php::object&>(cb_).prop("__CONNECTION_HANDLER__").is_true()) {
					cb_type = 2;
				}else{
					cb_type = 1;
				}
			}else{
				cb_type = 0;
				throw php::exception("handler must be callable");
			}
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
			if(close(true)) {
				co_ = coroutine::current;
				return flame::async();
			}else{
				return nullptr;
			}
		}
		bool close(bool stop_run) {
			if(closing_) return false;
			closing_ = true;
			uv_close((uv_handle_t*)&server, close_cb);
			if(stop_run && co_ != nullptr) { // 运行协程恢复
				co_->next();
			}
			co_ = nullptr;
			return true;
		}
	private:
		MY_SERVER_T*    self_;
		coroutine*        co_;
		php::value       ref_;
		int               cb_type;
		php::value        cb_;
		bool         closing_;
		static void connection_cb(uv_stream_t* handle, int error) {
			impl_t* self = reinterpret_cast<impl_t*>(handle->data);
			if(error < 0) {
				self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
				self->close(false);
				self->co_->fail(uv_strerror(error), error);
				return;
			}

			if(self->cb_type == 2) { // 允许使用内部对象接管客户端对象
				php::value svr(handle);  // 将服务器对象指针放入，用于 accept 连接
				error = reinterpret_cast<php::callable&>(self->cb_)(svr, handle->type);
				if(error < 0) {
					self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用3
					self->close(false);
					self->co_->fail(uv_strerror(error), error);
				}
			}else/* if(self->cb_type == 1) */{
				php::object  cli = php::object::create<MY_SOCKET_T>();
				MY_SOCKET_T* cpp = cli.native<MY_SOCKET_T>();
				error = uv_accept(handle, (uv_stream_t*)&cpp->impl->stream);
				if(error < 0) {
					self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用3
					self->close(false);
					self->co_->fail(uv_strerror(error), error);
					return;
				}
				cpp->after_init();
				coroutine::start(
					reinterpret_cast<php::callable&>(self->cb_), cli);
			}
		}
		static void close_cb(uv_handle_t* handle) {
			impl_t* self = reinterpret_cast<impl_t*>(handle->data);
			if(self->co_) {
				self->co_->next();
			}
			delete self;
		}
	};
}
}
