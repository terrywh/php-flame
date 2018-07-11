#pragma once

namespace flame {
namespace mysql {
	class result: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value fetch_row(php::parameters& params);
		php::value fetch_all(php::parameters& params);
	private:
		// 使用 store_result 获得 (实际释放过程可以脱离原客户端连接)
		std::shared_ptr<MYSQL_RES> r_;
		friend class _connection_base;
	};
}
}