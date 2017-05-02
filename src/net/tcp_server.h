#pragma once

namespace net {
	class tcp_server: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		tcp_server();
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value listen(php::parameters& params);
		php::value accept(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		tcp::acceptor     acceptor_;
		bool              is_ipv6_;
		bool              listened_;
		void set_prop_local_addr();
	};
}
