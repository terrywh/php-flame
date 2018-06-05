#pragma once

namespace flame {
namespace net {
	
template <class SOCKET_TYPE>
class server_acceptor {
public:
	server_acceptor(uv_stream_t* svr)
	: svr_(svr), cb_type(0) {
		svr_->data = this;
	}
	void handle(const php::callable& cb) {
		if(cb.is_callable()) {
			cb_ = cb;
			// 限定特殊类型标志
			if(cb_.is_object() && static_cast<php::object&>(static_cast<php::value&>(cb_)).prop("__CONNECTION_HANDLER__").is_true()) {
				cb_type = 2;
			}else{
				cb_type = 1;
			}
		}else{
			cb_type = 0;
			throw php::exception("handler must be callable");
		}
	}
	void run() {
		if(coroutine::current == nullptr) {
			throw php::exception("cannot run server (not in a coroutine?)");
		}
		int error = uv_listen((uv_stream_t*)svr_, 1024, connection_cb);
		if(error < 0) {
			close(error);
			throw php::exception(uv_strerror(error), error);
		}
		co_  = coroutine::current;
	}
	void close(int err = 0) {
		if(co_) {
			coroutine* co = co_;
			co_ = nullptr;
			// 由于 co->next() 本身可能引起下一次的读取动作，
			// 故 self->co_ = nullptr 必须置于 co->next() 之前
			// 否则可能导致将下次的 co_ 被清理
			err == 0 ? co->next() : co->fail(uv_strerror(err), err);
		}
	}
private:
	uv_stream_t*      svr_;
	coroutine*        co_;
	php::object       ref_;
	int               cb_type;
	php::callable     cb_;
	
	static void connection_cb(uv_stream_t* handle, int err) {
		server_acceptor* self = reinterpret_cast<server_acceptor*>(handle->data);
		if(err < 0) {
			self->close(err);
		}else if(self->cb_type == 2) { // 允许使用内部对象接管客户端对象
			php::value svr(handle);  // 将服务器对象指针放入，用于 accept 连接
			err = self->cb_.invoke({svr, handle->type});
			if(err < 0) {
				self->close(err);
			}
		}else/* if(self->cb_type == 1) */{
			php::object  obj = php::object::create<SOCKET_TYPE>();
			SOCKET_TYPE* cpp = obj.native<SOCKET_TYPE>();
			err = uv_accept(handle, (uv_stream_t*)cpp->sck);
			if(err < 0) {
				self->close(err);
				return;
			}
			cpp->after_init();
			coroutine::create(self->cb_, {obj})->start();
		}
	}
};

}
}
