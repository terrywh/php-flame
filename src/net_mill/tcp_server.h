#pragma once

namespace net {
	class tcp_server: public php::class_base {
	public:
		tcp_server():closed_(true) {}
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value accept(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		mill_tcpsock server_;
		bool         closed_;
		mill_ipaddr local_addr_;
	};
}
