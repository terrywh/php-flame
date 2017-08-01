#pragma once
#include "../fiber.h"

namespace flame {
namespace net {
	class stream_server: public php::class_base {
	public:
		stream_server(uv_stream_t* s);
		php::value __destruct(php::parameters& params);
		php::value run(php::parameters& params);		
		php::value close(php::parameters& params);
	private:
		
		static void connection_cb(uv_stream_t* stream, int error);
		static void close_cb(uv_handle_t* handle);
	protected:
		uv_stream_t*  pstream_;
		int           status_;
		virtual void accept(uv_stream_t* s) = 0;
		virtual uv_stream_t* create_stream() = 0;
	};
}
}