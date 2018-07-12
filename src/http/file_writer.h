#pragma once

namespace flame {
namespace http {
	template <bool isRequest>
	class value_body;
	class handler;
	class file_writer: public boost::asio::coroutine, public std::enable_shared_from_this<file_writer> {
	public:
		file_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, boost::filesystem::path path, int status);
		void start();
	private:
		std::shared_ptr<flame::coroutine>     co_;
		std::shared_ptr<handler>         handler_;
		boost::filesystem::path             path_;
		int                               status_;

		int                                   fd_;
		boost::asio::posix::stream_descriptor ds_;
		std::shared_ptr<boost::beast::http::serializer<false, value_body<false>>> sr_;
		char                              buffer_[4096];

		void write_not_found(const boost::system::error_code& error, std::size_t n);
		void write_file_data(const boost::system::error_code& error, std::size_t n);
	};
}
}
