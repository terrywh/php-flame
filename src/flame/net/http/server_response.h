#pragma once 

namespace flame {
namespace net {
namespace http {
	class server_request {
	public:
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		// property headers array()
	private:
		php::object    socket_;
		server_socket* psocket_;
	};
}
}
}