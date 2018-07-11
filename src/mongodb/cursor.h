#pragma once

namespace flame {
namespace mongodb {
	class _connection_lock;
	class cursor: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value next(php::parameters& params);
		php::value to_array(php::parameters& params);
	private:
		std::shared_ptr<_connection_lock> p_;
		std::shared_ptr<mongoc_cursor_t>  c_;
		friend class _connection_base;
	};
}
}