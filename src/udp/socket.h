#pragma once

namespace flame {
	class coroutine;
namespace udp {
	class socket: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		socket();
		php::value __construct(php::parameters& params);
		php::value receive(php::parameters& params);
		php::value receive_from(php::parameters& params);
		php::value send(php::parameters& params);
		php::value send_to(php::parameters& params);
		php::value close(php::parameters& params);
		void write_ex();
	private:
		boost::asio::ip::udp::socket socket_;
		php::buffer buffer_;

		friend php::value connect(php::parameters& params);
	};
}
}