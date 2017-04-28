#pragma once

namespace net {
	class udp_socket: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		udp_socket();
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value bind(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		php::value remote_port(php::parameters& params);
		php::value close(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write_to(php::parameters& params);
		php::value write(php::parameters& params);
	private:
		udp::socket     socket_;
		char            buffer_[64*1024];
		// 最近一次接收的来源
		udp::endpoint   remote_;
		bool            connected_;
		void init_local_prop();
	};
}
