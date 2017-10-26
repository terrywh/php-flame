#include "messenger.h"
#include "../coroutine.h"

namespace flame {
namespace os {
	messenger::messenger() {
		uv_pipe_init(flame::loop, &pipe_, 1);
		uv_pipe_open(&pipe_, 0);
		pipe_.data = this;
	}
	void messenger::start() {
		uv_read_start((uv_stream_t*)&pipe_, alloc_cb, read_cb);
	}
	void messenger::alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf) {
		static char buffer[2048];
		buf->base = buffer;
		buf->len  = sizeof(buffer);
	}
	void messenger::read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
		messenger* m = static_cast<messenger*>(handle->data);
		if(nread < 0) {
			std::printf("failed to read: %d %s\n", nread, uv_strerror(nread));
		}else if(nread == 0) {

		}else{
			std::printf("--(%d)[%.*s]--\n", nread, nread-4, buf->base + 4);
			// TODO 解析进程通讯数据
			// 接收过程参照 server_handler 流程，区分消息、句柄两种形式
		}
	}
	static messenger* msg = nullptr;
	php::value set_message_handler(php::parameters& params) {
		if(msg == nullptr) {
			msg = new messenger();
			msg->start();
		}
		msg->cb_socket = params[0];
		return nullptr;
	}
	php::value set_socket_handler(php::parameters& params) {
		if(msg == nullptr) {
			msg = new messenger();
			msg->start();
		}
		msg->cb_message = params[0];
		return nullptr;
	}
}
}
