#include "net.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	void init(php::extension_entry& ext) {
		// class_tcp_socket
		// ------------------------------------
		php::class_entry<tcp_socket> class_tcp_socket("flame\\net\\tcp_socket");
		class_tcp_socket.add<&tcp_socket::connect>("connect");
		ext.add(std::move(class_tcp_socket));
		// class_tcp_socket
		// ------------------------------------
	}
}	
}
