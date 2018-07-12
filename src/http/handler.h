#pragma once

namespace flame {
namespace http {
	template <bool isRequest>
    class value_body;
	class server;
	class handler: public boost::asio::coroutine, public std::enable_shared_from_this<handler> {
	public:
		handler(server* svr, tcp::socket&& sock, std::shared_ptr<flame::coroutine> co);
		~handler();
		void handle(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		void write_header(std::shared_ptr<flame::coroutine> co);
	private:
		server*        svr_;
		tcp::socket socket_;
		std::shared_ptr<boost::beast::http::message<true, value_body<true>>>   req_;
		std::shared_ptr<boost::beast::http::message<false, value_body<false>>> res_;
		std::shared_ptr<flame::coroutine> co_;
		boost::beast::flat_buffer     buffer_;
		friend class server_response;
		friend class server_request;
		friend class chunked_writer;
		friend class file_writer;
		friend class content_writer;
	};
	enum {
		RESPONSE_STATUS_HEADER_BUILT = 0x01,
		RESPONSE_STATUS_HEADER_SENT  = 0x02,
		RESPONSE_STATUS_FINISHED     = 0x04,
		RESPONSE_STATUS_DETACHED     = 0x08,

		RESPONSE_TARGET_WRITE_HEADER = 0x01,
		RESPONSE_TARGET_WRITE_CHUNK  = 0x02,
		RESPONSE_TARGET_WRITE_CHUNK_LAST = 0x03,
	};
}
}
