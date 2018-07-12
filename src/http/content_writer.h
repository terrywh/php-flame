#pragma once

namespace flame {
namespace http {
	class handler;
	class content_writer: public boost::asio::coroutine, public std::enable_shared_from_this<content_writer> {
	public:
		content_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, php::string body, int status);
		void start();
		// virtual ~content_writer();
	private:
        std::shared_ptr<flame::coroutine> co_;
        std::shared_ptr<handler>     handler_;
        php::string                     body_;
        int                           status_;

		void write(const boost::system::error_code& error, std::size_t n);

		friend class server_response;
	};
}
}
