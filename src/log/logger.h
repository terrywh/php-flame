#pragma once

namespace flame {
namespace log {
	class logger: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		virtual ~logger();
		// C++ 内部调用
		void initialize();
		void write(const php::string& data);
		void rotate();
		// PHP 
		php::value __construct(php::parameters& params);
		php::value write(php::parameters& params);
		php::value info(php::parameters& params);
		php::value warn(php::parameters& params);
		php::value fail(php::parameters& params);
		php::value rotate(php::parameters& params);
		bool to_file();
	private:
		std::string                                         fpath_;
		char                                                qname_[33];
		std::unique_ptr<boost::interprocess::message_queue> queue_;
		std::thread                                         write_;
		std::unique_ptr<boost::asio::signal_set>           signal_;
		std::list<php::object>                             master_;
		constexpr static int MESSAGE_MAX_COUNT = 256;
		constexpr static int MESSAGE_MAX_SIZE  = 32 * 1024;

		void write_ex(std::ostream& os, php::parameters& params);
		void on_sigusr2(const boost::system::error_code& error, int sig);
	};
}
}