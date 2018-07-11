#pragma once

namespace flame {
	class coroutine;
namespace tcp {
	class socket: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		socket();
		php::value read(php::parameters& param);
		php::value write(php::parameters& params);
		php::value close(php::parameters& params);
		void write_ex();
	private:
		boost::asio::ip::tcp::socket socket_;
		php::buffer buffer_;
		std::list< std::pair<std::shared_ptr<coroutine>, php::string> > q_;

		friend class acceptor;
		friend php::value connect(php::parameters& params);
	};
}
}