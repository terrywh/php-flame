#pragma once

namespace flame {
namespace log {
	class logger: public php::class_base {
	public:
		logger();
		~logger();
		php::value set_output(php::parameters& params);
		void rotate(const php::string& path);
		void rotate();
		void init() {
			rotate();
		}
		php::value fail(php::parameters& params);
		php::value warn(php::parameters& params);
		php::value info(php::parameters& params);
		php::value write(php::parameters& params);
		
		bool write(const std::string& data);
		bool write(const php::string& out);
		void close();
		// 特殊函数，用于在进程发生“panic”时记录错误信息（同步）
		void panic();
	private:
		uv_pipe_t*   pipe_;
		php::string  path_;
		int          file_;
		uv_signal_t*  sig_;
		static void write_cb(uv_write_t* req, int status);
		static void signal_cb(uv_signal_t* handle, int signal);
		php::string make_buffer(const std::string& level, php::parameters& params);
		void panic_to_file(php::object& ex);
	};
}
}
