#pragma once

namespace flame {
namespace tcp {
	class server;
	class acceptor: public boost::asio::coroutine, public std::enable_shared_from_this<acceptor> {
	public:
		acceptor(std::shared_ptr<flame::coroutine> co, server* svr);
		void accept(const boost::system::error_code& error = boost::system::error_code());
	private:
		std::shared_ptr<flame::coroutine> co_;
		server*       svr_;
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket   socket_;

		friend class server;
	};
}
}