#include "../vendor.h"
#include "addr.h"

namespace net {

	php::value addr_t::__toString(php::parameters& params) {
		php::value r = php::value::string(MILL_IPADDR_MAXSTRLEN + 6);
		zend_string* buffer = r;
		mill_ipaddrstr(addr_, buffer->val);
		buffer->len  = std::strlen(buffer->val);
#define PHP_SPRINTF sprintf
#undef sprintf
		buffer->len += std::sprintf(buffer->val + buffer->len, ":%d", port_);
#define sprintf PHP_SPRINTF
		return std::move(r);
	}

	php::value addr_t::port(php::parameters& params) {
		return port_;
	}

	php::value addr_t::host(php::parameters& params) {
		php::value r = php::value::string(MILL_IPADDR_MAXSTRLEN);
		zend_string* buffer = r;
		mill_ipaddrstr(addr_, buffer->val);
		buffer->len  = std::strlen(buffer->val);
		return std::move(r);
	}
}
