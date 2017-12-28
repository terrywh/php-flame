#pragma once
#include "../io/stream_reader.h"
#include "../io/stream_writer.h"

namespace flame {
namespace net {
	class unix_socket: public php::class_base {
	public:
		unix_socket();
		~unix_socket();
		php::value connect(php::parameters& params);
		php::value read(php::parameters& params);
		php::value read_all(php::parameters& params);
		php::value write(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address ""
		// property remote_address ""
		void close(int err = 0);
		void after_init();
		uv_pipe_t*        sck;
		io::stream_reader rdr;
		io::stream_writer wtr;
		
		int flags;
		enum {
			CAN_READ  = 0x01,
			CAN_WRITE = 0x02,
		};
	private:
		static void connect_cb(uv_connect_t* req, int status);
	};
}
}
