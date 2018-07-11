#pragma once

namespace flame {
	class coroutine;
namespace http {
	class server;
	class server_request;
	class server_response;
	class handler: public boost::asio::coroutine, public std::enable_shared_from_this<handler> {
	public:
		handler(server* svr, tcp::socket&& sock, std::shared_ptr<flame::coroutine> co);
		void handle(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		void write_header(std::shared_ptr<flame::coroutine> co);
	private:
		server* svr_;
		tcp::socket socket_;
		std::shared_ptr<flame::coroutine> co_;
		php::object     req_ref;
		server_request* req_;
		boost::beast::flat_buffer buffer_;
		php::object      res_ref;
		server_response* res_;
		friend class server_response;
		friend class chunked_writer;
		friend class file_writer;
	};
}
}