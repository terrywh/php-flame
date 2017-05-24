#pragma once

namespace db {
	class lmdb: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value get(php::parameters& params);
		php::value has(php::parameters& params);
		php::value set(php::parameters& params);
		php::value del(php::parameters& params);
		php::value incr(php::parameters& params);
		php::value flush(php::parameters& params);
		// Iterator
		php::value current(php::parameters& params);
		php::value key(php::parameters& params);
		php::value next(php::parameters& params);
		php::value rewind(php::parameters& params);
		php::value valid(php::parameters& params);
	private:
		MDB_env*    ev_;
		MDB_dbi     db_;
		MDB_cursor* cs_;
		MDB_val     key_;
		bool        end_;
	};
}
