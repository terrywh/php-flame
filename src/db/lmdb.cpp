#include "../vendor.h"
#include "lmdb.h"

namespace db {
	struct data_wrapper {
		zval        val;
		zend_string str;
	};
	void lmdb::init(php::extension_entry& extension) {
		php::class_entry<lmdb> class_lmdb("flame\\db\\lmdb");
		class_lmdb.implements_iterator();
		class_lmdb.add<&lmdb::__construct>("__construct");
		class_lmdb.add<&lmdb::__destruct>("__destruct");
		class_lmdb.add<&lmdb::has>("has");
		class_lmdb.add<&lmdb::get>("get");
		class_lmdb.add<&lmdb::set>("set");
		class_lmdb.add<&lmdb::del>("del");
		class_lmdb.add<&lmdb::flush>("flush");
		class_lmdb.add<&lmdb::incr>("incr");

		class_lmdb.add<&lmdb::current>("current");
		class_lmdb.add<&lmdb::key>("key");
		class_lmdb.add<&lmdb::next>("next");
		class_lmdb.add<&lmdb::rewind>("rewind");
		class_lmdb.add<&lmdb::valid>("valid");
		extension.add(std::move(class_lmdb));
	}
	static void throw_if_error(int err) {
		if(err == 0) return;
		throw php::exception(
			(boost::format("lmdb failed: %s") % mdb_strerror(err)).str(),
			err
		);
	}
	php::value lmdb::__construct(php::parameters& params) {
		int err = 0;
		throw_if_error( mdb_env_create(&ev_) );
		zend_string* path = params[0];
		throw_if_error( mdb_env_open(ev_, path->val, MDB_NOSUBDIR | MDB_WRITEMAP | MDB_MAPASYNC, 0664) );
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		mdb_dbi_open(tx, nullptr, 0, &db_);
		mdb_txn_commit(tx);
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		mdb_cursor_open(tx, db_, &cs_);
		mdb_txn_commit(tx);
		return nullptr;
	}

	php::value lmdb::__destruct(php::parameters& params) {
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		mdb_cursor_renew(tx, cs_);
		mdb_cursor_close(cs_);
		mdb_dbi_close(ev_, db_);
		mdb_txn_commit(tx);
		mdb_env_close(ev_);
		return nullptr;
	}
	static size_t size_from_rv(const php::value& rv) {
		switch(rv.type()) {
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
			case IS_LONG:
			case IS_DOUBLE:
				return sizeof(php::value);
			case IS_STRING:
				return sizeof(php::value) + sizeof(zend_string) + rv.length();
			default:
				throw php::exception("set failed: only null/boolean/long/double/string is allowed as value");
		}
	}
	static void rv_to_mv(php::value& rv, MDB_val& mv) {
		data_wrapper* dw;
		zend_string*  php_val;
		switch(rv.type()) {
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
			case IS_LONG:
			case IS_DOUBLE:
				mv.mv_size = sizeof(php::value);
				dw = reinterpret_cast<data_wrapper*>(mv.mv_data);
				std::memcpy(&dw->val, static_cast<zval*>(rv), sizeof(dw->val));
			break;
			case IS_STRING:
				dw = reinterpret_cast<data_wrapper*>(mv.mv_data);
				php_val = rv;
				std::memcpy(&dw->str, php_val, sizeof(zend_string) + rv.length());
				ZVAL_STR(&dw->val, &dw->str);
			break;
		}
	}

	php::value lmdb::set(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		MDB_val mdb_key, mdb_val;
		zend_string* php_key = params[0];
		// TODO 考虑将 php_val 复制到 mdb_val offset(zval) 后，并将 zval 指针指向对应地址
		// 以提高效率
		mdb_key.mv_size = php_key->len;
		mdb_key.mv_data = php_key->val;
		mdb_val.mv_size = size_from_rv(params[1]);
		mdb_put(tx, db_, &mdb_key, &mdb_val, MDB_RESERVE);
		rv_to_mv(params[1], mdb_val);
		mdb_txn_commit(tx);
		return nullptr;
	}

	php::value lmdb::has(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		MDB_val mdb_key, mdb_val;
		zend_string* php_key = params[0];
		mdb_key.mv_size = php_key->len;
		mdb_key.mv_data = php_key->val;
		err = mdb_get(tx, db_, &mdb_key, &mdb_val);
		mdb_txn_commit(tx);
		return err == 0;
	}
	static void mv_to_rv(const MDB_val& mdb_val, php::value& rv) {
		data_wrapper* dw = reinterpret_cast<data_wrapper*>(mdb_val.mv_data);
		switch(Z_TYPE(dw->val)) {
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
			case IS_LONG:
			case IS_DOUBLE:
				rv = dw->val;
			break;
			case IS_STRING:
				rv = php::string(dw->str.val, dw->str.len);
			break;
		}
	}
	php::value lmdb::get(php::parameters& params) {
		int err = 0;
		php::value rv = nullptr;
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		MDB_val mdb_key, mdb_val;
		zend_string* php_key = params[0];
		mdb_key.mv_size = php_key->len;
		mdb_key.mv_data = php_key->val;
		err = mdb_get(tx, db_, &mdb_key, &mdb_val);
		if(err == 0) {
			mv_to_rv(mdb_val, rv);
		}
		mdb_txn_commit(tx);
		return std::move(rv);
	}

	php::value lmdb::del(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		MDB_cursor* cs;
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		MDB_val mdb_key, mdb_val;
		zend_string* php_key = params[0];
		mdb_key.mv_size = php_key->len;
		mdb_key.mv_data = php_key->val;
		if(mdb_key.mv_size == key_.mv_size && std::memcmp(mdb_key.mv_data, key_.mv_data, mdb_key.mv_size) == 0) {
			// 若当前删除的是循环当前 key 需要将 当前 key 重置为前置 key
			mdb_cursor_open(tx, db_, &cs);
			err = mdb_cursor_get(cs, &key_, &mdb_val, MDB_SET);
			err = mdb_cursor_get(cs, &key_, &mdb_val, MDB_PREV);
			mdb_cursor_close(cs);
		}
		err = mdb_del(tx, db_, &mdb_key, nullptr);
		mdb_txn_commit(tx);
		return err == 0;
	}
	php::value lmdb::flush(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		// 删除 db 并重新建立
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		err = mdb_drop(tx, db_, 1);
		err = mdb_dbi_open(tx, nullptr, 0, &db_);
		mdb_txn_commit(tx);
		// 重新打开 cursor
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		mdb_cursor_open(tx, db_, &cs_);
		mdb_txn_commit(tx);
		return err == 0;
	}
	php::value lmdb::incr(php::parameters& params) {
		php::value rv = params[1];
		if(!rv.is_long()) {
			throw php::exception("incr failed: only long number is allowed as value");
		}
		int err = 0;
		MDB_txn* tx;
		mdb_txn_begin(ev_, nullptr, 0, &tx);
		MDB_val mdb_key, mdb_val;
		zend_string* php_key = params[0];
		mdb_key.mv_size = php_key->len;
		mdb_key.mv_data = php_key->val;
		err = mdb_get(tx, db_, &mdb_key, &mdb_val);
		mdb_val.mv_size = size_from_rv(rv);
		if(err == 0) { // 已存在的 key 获取当前值
			php::value php_val;
			mv_to_rv(mdb_val, php_val);
			// TODO 若当前值类型不正确怎么办？
			mdb_put(tx, db_, &mdb_key, &mdb_val, MDB_RESERVE);
			rv = static_cast<std::int64_t>(php_val) + static_cast<std::int64_t>(rv);
			rv_to_mv(rv, mdb_val);
		}else{ // 一般为 key 不存在的情况
			mdb_put(tx, db_, &mdb_key, &mdb_val, MDB_RESERVE);
			rv_to_mv(rv, mdb_val); // 设置默认值
		}
		mdb_txn_commit(tx);
		return std::move(rv);
	}
	php::value lmdb::rewind(php::parameters& params) {
		int err = 0;
		MDB_val mdb_val;
		MDB_txn*    tx;
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		mdb_cursor_renew(tx, cs_);
		err = mdb_cursor_get(cs_, &key_, &mdb_val, MDB_FIRST);
		if(err != 0) {
			end_ = true;
		}else{
			end_ = false;
		}
		// mdb_cursor_close(cs_);
		mdb_txn_commit(tx);
		return nullptr;
	}
	php::value lmdb::valid(php::parameters& params) {
		return !end_;
	}
	// Iterator
	php::value lmdb::current(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		MDB_val mdb_val;
		php::value  rv;
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		mdb_cursor_renew(tx, cs_);
		err = mdb_cursor_get(cs_, &key_, &mdb_val, MDB_SET_KEY);
		if(err == 0) {
			mv_to_rv(mdb_val, rv);
		}
		// mdb_cursor_close(cs_);
		mdb_txn_commit(tx);
		return std::move(rv);
	}
	php::value lmdb::key(php::parameters& params) {
		php::string key(reinterpret_cast<char*>(key_.mv_data), key_.mv_size);
		return std::move(key);
	}
	php::value lmdb::next(php::parameters& params) {
		int err = 0;
		MDB_txn* tx;
		MDB_val mdb_val;
		mdb_txn_begin(ev_, nullptr, MDB_RDONLY, &tx);
		mdb_cursor_renew(tx, cs_);
		err = mdb_cursor_get(cs_, &key_, &mdb_val, MDB_SET);
		err = mdb_cursor_get(cs_, &key_, &mdb_val, MDB_NEXT);
		// mdb_cursor_close(cs_);
		mdb_txn_commit(tx);
		end_ = (err != 0);
		return nullptr;
	}
}
