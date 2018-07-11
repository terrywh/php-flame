#pragma once

namespace flame {
	class coroutine;
namespace tcp {
	class acceptor;
	class server: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		boost::asio::ip::tcp::endpoint addr_;
		php::callable cb_;
		std::shared_ptr<acceptor> acc_;

		friend class acceptor;
	};
}
}