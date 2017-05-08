#pragma once

namespace db {
	// connection 负责管理 mysql 、redis、mongodb 连接维护连接的活跃（防止服务端断开）
	class connection: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		static php::value preserve(php::parameters& params);
		~connection();
	private:
		std::vector<boost::asio::steady_timer*> timer_;
		static void await_timer(boost::asio::steady_timer* tmr, php::object obj, int itv, const std::string& fn);
	};
}
