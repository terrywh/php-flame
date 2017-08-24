#pragma once
#include "../fiber.h"

namespace flame {
namespace net {
	class stream_server: public php::class_base {
	public:
		stream_server(uv_stream_t* s);
		virtual ~stream_server();
		php::value run_core(php::parameters& params);
		php::value run_unix(php::parameters& params);
		php::value close(php::parameters& params);
	private:

		static void connection_cb_core(uv_stream_t* server, int error);
		static void connection_cb_unix_master(uv_stream_t* server, int error);
		static void connection_cb_unix_worker(uv_stream_t* server, int error, void* ctx);
		static void close_cb(uv_handle_t* handle);
	protected:
		uv_stream_t*  pstream_;
		// 注意：次函数接收的 server 可能与当前 pstream_ 不同
		virtual int accept(uv_stream_t* server) = 0;
		bool          running_;
	};
}
}
