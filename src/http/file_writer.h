#pragma once

namespace flame {
	class coroutine;
namespace http {
	class server_response;
	class file_writer: public boost::asio::coroutine, public std::enable_shared_from_this<file_writer> {
	public:
		file_writer(server_response* res, std::shared_ptr<flame::coroutine> co, boost::filesystem::path path);
		void write(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		void start();
	private:
		server_response* res_;
		std::shared_ptr<flame::coroutine> co_;
		boost::filesystem::path path_;
		int fd_;
		boost::asio::posix::stream_descriptor ds_;
		char buffer_[4096];
		friend class server_response;
	};
}
}