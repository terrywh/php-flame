#pragma once

namespace flame {
namespace os {
	php::value set_message_handler(php::parameters& params);
	class messenger {
	public:
		messenger();
		void start(const php::value& cb);
	private:
		uv_pipe_t   pipe_;
		php::callable cb_;
		php::buffer  buf_;
		static void alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf);
		static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
	};
}
}
