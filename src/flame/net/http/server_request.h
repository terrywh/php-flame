#pragma once 

namespace flame {
namespace net {
namespace http {
	class server_request {
	public:
		// property headers null
		// property method  ""
		// property version ""
		// property query null
		// property body  null
	private:
		php::object    socket_;
		server_socket* psocket_;
	};
}
}
}