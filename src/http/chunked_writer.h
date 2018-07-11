#pragma once

namespace flame {
	class coroutine;
namespace http {
	class server_response;
	class chunked_writer: public boost::asio::coroutine, public std::enable_shared_from_this<chunked_writer> {
	public:
		chunked_writer(server_response* res, std::shared_ptr<flame::coroutine> co);
		void write(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		void start(int step);
		void start(int step, const php::string& chunk);
		// virtual ~chunked_writer();
	private:
		server_response* res_;
		std::shared_ptr<flame::coroutine> co_;
		int step_;
		bool writing_;
		enum {
			STEP_WRITE_HEADER,
			STEP_WRITE_CHUNK,
			STEP_WRITE_CHUNK_LAST,
		};
		php::string chunk_;
		friend class server_response;
	};
}
}