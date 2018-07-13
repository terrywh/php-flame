#pragma once

namespace flame {
namespace log {
	class logger: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		logger();
		virtual ~logger();
		// C++ 内部调用
		std::string write(boost::format& data);
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
		// 公共
		std::string                                                fname_;
		std::string                                                fpath_;
		static std::unique_ptr<boost::interprocess::message_queue> queue_;
		// 主进程
		std::thread                                         writer_;
		std::unique_ptr<boost::asio::signal_set>            signal_;
		std::map<std::string, std::pair<std::string, std::shared_ptr<std::ostream>>> file_;
		constexpr static int MESSAGE_MAX_COUNT = 512;
		constexpr static int MESSAGE_MAX_SIZE  = 24 * 1024;

		void write_ex(std::ostream& os, php::parameters& params);
		void on_sigusr2(const boost::system::error_code& error, int sig);
		void writer();
		void rotate_ex(std::pair<std::string, std::shared_ptr<std::ostream>>& file);
	};
}
}
