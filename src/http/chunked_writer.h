#pragma once

namespace flame {
namespace http {
	template <bool isRequest>
	class value_body;
	class handler;
	class chunked_writer: public boost::asio::coroutine, public std::enable_shared_from_this<chunked_writer> {
	public:
		chunked_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, int target, int status);
		chunked_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, int target, const php::string& chunk, int status);

		void start();
		// virtual ~chunked_writer();
	private:
		std::shared_ptr<flame::coroutine>     co_;
		std::shared_ptr<handler>         handler_;
		int                               target_;
		int                               status_;
		std::shared_ptr<boost::beast::http::serializer<false, value_body<false>>> sr_;
		php::string                        chunk_;

		void write(const boost::system::error_code& error, std::size_t n);
		friend class server_response;
	};
}
}
