#include "server_request.h"

namespace flame {
namespace net {
namespace http {

server_request::server_request() {
	prop("header") = php::array(4);
	// prop("query")  = php::array(0);
	// prop("cookie") = php::array(0);
}

}
}
}
