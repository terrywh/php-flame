#pragma once

namespace net {
	class tcp_server;
    namespace http { class request; class response; }
	class tcp_socket: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		tcp_socket(bool connected = false);
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		php::value remote_port(php::parameters& params);
		php::value close(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write(php::parameters& params);
	private:
		tcp::socket     socket_;
		boost::asio::streambuf buffer_;
		tcp::endpoint   remote_;
		bool            connected_;
		bool            is_ipv6_;
		void set_prop_local_addr();
		friend class tcp_server;
        friend class http::request;
        friend class http::response;
	};
}
