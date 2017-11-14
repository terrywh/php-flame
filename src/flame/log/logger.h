#pragma once

namespace flame {
namespace log {
	class logger: public php::class_base {
	public:
		logger();
		~logger();
		php::value rotate(php::parameters& params);
		void rotate(const php::string& path);
		void rotate();
		void init() {
			rotate();
		}
		php::value fail(php::parameters& params);
		php::value warn(php::parameters& params);
		php::value info(php::parameters& params);
		php::value write(php::parameters& params);
		void write(const std::string& str);
		void close();
	private:
		uv_pipe_t*   pipe_;
		php::string  path_;
		int          file_;
		static void write_cb(uv_write_t* req, int status);
		php::value write(const std::string& level, php::parameters& params);
	};
}
}
