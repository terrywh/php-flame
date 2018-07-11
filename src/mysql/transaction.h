#pragma once

namespace flame {
namespace mysql {
	class _connection_lock;
	class transaction: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value commit(php::parameters& params);
		php::value rollback(php::parameters& params);

		php::value escape(php::parameters& params);
		php::value query(php::parameters& params);
		
		php::value insert(php::parameters& params);
		php::value delete_(php::parameters& params);
		php::value update(php::parameters& params);
		php::value select(php::parameters& params);
		php::value one(php::parameters& params);
		php::value get(php::parameters& params);
	protected:
		std::shared_ptr<_connection_lock> c_;
		void query(std::shared_ptr<coroutine> co, const php::string& sql);
		friend class client;
	};
}
}