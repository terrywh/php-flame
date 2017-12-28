#pragma once
#include "../io/stream_reader.h"
#include "../io/stream_writer.h"

namespace flame {
namespace net {
	class tcp_socket: public php::class_base {
	public:
		tcp_socket();
		~tcp_socket();
		php::value connect(php::parameters& params);
		php::value read(php::parameters& params);
		php::value read_all(php::parameters& params);
		php::value write(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address ""
		// property remote_address ""
		void close();
		void after_init();
		uv_tcp_t*         sck;
		io::stream_reader rdr;
		io::stream_writer wtr;
	private:
		static void connect_cb(uv_connect_t* req, int status);
	};
}
}
