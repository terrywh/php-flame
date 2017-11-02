#include "../../flame.h"
#include "../../coroutine.h"
#include "client.h"
#include "result_set.h"
#include "mysql.h"

#define MYSQL_PING_INTERVAL 10000
namespace flame {
namespace db {
namespace mysql {
	void client::value_to_buffer(php::value& val, php::buffer& buf) {
		if(val.is_null()) {
			std::memcpy(buf.put(4), "NULL", 4);
		}else if(val.is_bool()) {
			if(val.is_true()) std::memcpy(buf.put(4), "TRUE", 4);
			else std::memcpy(buf.put(5), "FALSE", 5);
		}else if(val.is_string()) {
			buf.add('\'');
			php::string& str = val;
			char*        esp = buf.rev(str.length() * 2);
			size_t       len = mysqlnd_real_escape_string(mysql_, esp,
				str.data(), str.length());
			buf.adv(len);
			buf.add('\'');
		}else if(val.is_long()) {
			long   x = val;
			size_t n = sprintf(buf.rev(10), "%ld", x);
			buf.adv(n);
		}else if(val.is_array()) {
			php::array& row = val;
			int j = -1;
			for(auto i=row.begin(); i!= row.end(); ++i) {
				if(++j == 0) buf.add('(');
				else buf.add(',');
				value_to_buffer(i->second, buf);
			}
			buf.add(')');
		}else{
			buf.add('\'');
			php::string  str = val.to_string();
			char*        esp = buf.rev(str.length() * 2);
			size_t       len = mysqlnd_real_escape_string(mysql_, esp,
				str.data(), str.length());
			buf.adv(len);
			buf.add('\'');
		}
	}
	typedef struct mysql_request_t {
		coroutine*    co;
		MYSQLND*   mysql;
		php::value    rv;
		php::string  sql;
		uv_work_t    req;
	} mysql_request_t;
	client::client()
	: mysql_(nullptr) {

	}
	php::value client::__destruct(php::parameters& params) {
		if(mysql_) {
			close(params);
		}
		return nullptr;
	}
	void default_cb(uv_work_t* req, int status) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void client::connect_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(ctx->mysql) mysqlnd_close(ctx->mysql, MYSQLND_CLOSE_EXPLICIT);
		php::object& obj = ctx->rv;
		client* self = obj.native<client>();
		ctx->mysql = mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, true);
		ctx->mysql = mysqlnd_connect(
			ctx->mysql,
			self->url_->host,
			self->url_->user,
			self->url_->pass,
			std::strlen(self->url_->pass),
			self->url_->path+1,
			std::strlen(self->url_->path) - 1,
			self->url_->port,
			nullptr,
			0,
			MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA);
		if(ctx->mysql) {
			self->mysql_ = ctx->mysql;
			mysqlnd_autocommit(ctx->mysql, true);
			ctx->rv = (bool)true;
		}else{
			self->mysql_ = nullptr;
			ctx->rv = php::make_exception(mysqlnd_error(ctx->mysql), mysqlnd_errno(ctx->mysql));
			mysqlnd_close(ctx->mysql, MYSQLND_CLOSE_DISCONNECTED);
		}
	}
	void client::connect_() {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this,
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, connect_wk, default_cb);
	}
	php::value client::connect(php::parameters& params) {
		if(params.length() > 0) {
			php::string& uri = params[0];
			url_ = php::parse_url(uri.c_str(), uri.length());
			if(std::strncmp(url_->scheme, "mysql", 5) != 0 || std::strlen(url_->path) < 1) {
				throw php::exception("illegal connection uri");
			}
			if(!url_->port) {
				url_->port = 3306;
			}
		}
		connect_();
		// // 安排进行 5 分钟 ping 防止 gone away 问题
		// ping_ = (ping_request_t*)malloc(sizeof(ping_request_t));
		// ping_->self     = this;
		// ping_->req.data = ping_;
		// uv_timer_init(flame::loop, &ping_->req);
		// uv_timer_start(&ping_->req, ping_cb, MYSQL_PING_INTERVAL, 0);
		// uv_unref((uv_handle_t*)&ping_->req);
		return flame::async();
	}
	void client::close_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		mysqlnd_close(ctx->mysql, MYSQLND_CLOSE_EXPLICIT);
		ctx->rv = true;
	}
	php::value client::close(php::parameters& params) {
		if(mysql_) {
			mysql_request_t* ctx = new mysql_request_t {
				coroutine::current, mysql_, this
			};
			mysql_ = nullptr;
			ctx->req.data = ctx;
			uv_queue_work(flame::loop, &ctx->req, close_wk, default_cb);
			return flame::async();
		}
		return nullptr;
	}
	php::value client::format(php::parameters& params) {
		php::string sql = params[0];
		int         idx = 0;
		php::buffer buf;
		for(char* c = sql.data(); c != sql.data() + sql.length(); ++c) {
			if(*c == '?') {
				++idx;
				value_to_buffer(params[idx], buf);
			}else{
				buf.add(*c);
			}
		}
		return std::move(buf);
	}
	void client::query_(php::buffer& sql) {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this
		};
		ctx->sql = std::move(sql);
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, query_wk, default_cb);
	}
	void client::query_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		int error = mysqlnd_query(ctx->mysql, ctx->sql.c_str(), ctx->sql.length());
		if(error != 0) {
			ctx->rv = php::make_exception(mysqlnd_error(ctx->mysql), error);
			return;
		}
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->mysql);
		if(rs) {
			php::object obj = php::object::create<result_set>();
			result_set* cpp = obj.native<result_set>();
			cpp->init(ctx->rv, rs);
			ctx->rv = std::move(obj);
			return;
		}
		if(mysqlnd_field_count(ctx->mysql) == 0) {
			reinterpret_cast<php::object&>(ctx->rv).prop("affected_rows") = mysqlnd_affected_rows(ctx->mysql);
			ctx->rv = (bool)true;
			return;
		}
		ctx->rv = php::make_exception(mysqlnd_error(ctx->mysql), mysqlnd_errno(ctx->mysql));
	}
	php::value client::query(php::parameters& params) {
		php::string sql = params[0];
		if(params.length() > 1) {
			sql = format(params);
		}
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this, params[0].to_string()
		};
		if(params.length() > 1) {
			ctx->sql = format(params);
		}
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, query_wk, default_cb);
		return flame::async();
	}
	void client::insert_key(php::array& map, php::buffer& buf) {
		int j = -1;
		for(auto i= map.begin(); i!= map.end(); ++i) {
			php::string key = i->first.to_string();
			if(++j == 0) buf.add('(');
			else buf.add(',');
			buf.add('`');
			std::memcpy(buf.put(key.length()), key.data(), key.length());
			buf.add('`');
		}
		buf.add(')');
	}
	php::value client::insert(php::parameters& params) {
		php::string& table = params[0];
		php::array&  data  = params[1];

		php::buffer sql;
		std::memcpy(sql.put(13), "INSERT INTO `", 13);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		sql.add(' ');
		if(data.is_a_map()) {
			insert_key(data, sql);
			std::memcpy(sql.put(8), " VALUES ", 8);
			value_to_buffer(data, sql);
		} else {
			insert_key(data[0], sql);
			std::memcpy(sql.put(8), " VALUES ", 8);
			int j = -1;
			for(auto i= data.begin(); i!= data.end(); ++i) {
				if(++j > 0) sql.add(',');
				value_to_buffer(i->second, sql);
			}
		}
		query_(sql);
		return flame::async();
	}
	php::value client::remove(php::parameters& params) {
		php::string& table = params[0];
		php::buffer sql;
		std::memcpy(sql.put(13), "DELETE FROM `", 13);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		sql_where(this, params[1], sql);
		if(params.length() > 2) {
			sql_orderby(this, params[2], sql);
		}
		if(params.length() > 3) {
			sql_limit(this, params[3], sql);
		}
		query_(sql);
		return flame::async();
	}
	php::value client::update(php::parameters& params) {
		php::string& table = params[0];
		php::array&  data  = params[2];
		php::buffer  sql;
		std::memcpy(sql.put(8), "UPDATE `", 8);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		std::memcpy(sql.put(7), "` SET", 5);
		int j = -1;
		for(auto i=data.begin(); i!=data.end(); ++i) {
			php::string& key = i->first;
			if(++j == 0) sql.add(' ');
			else sql.add(',');
			sql.add('`');
			std::memcpy(sql.put(key.length()), key.c_str(), key.length());
			sql.add('`');
			sql.add('=');
			value_to_buffer(i->second, sql);
		}
		sql_where(this, params[1], sql);
		if(params.length() > 2) {
			sql_orderby(this, params[2], sql);
		}
		if(params.length() > 3) {
			sql_limit(this, params[3], sql);
		}
		query_(sql);
		return flame::async();
	}
	void client::one_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		int error = mysqlnd_query(ctx->mysql, ctx->sql.c_str(), ctx->sql.length());
		if(error != 0) {
			ctx->rv = php::make_exception(mysqlnd_error(ctx->mysql), error);
			return;
		}
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->mysql);
		if(rs) {
			mysqlnd_fetch_into(rs, MYSQLND_FETCH_NUM, (zval*)&ctx->rv, MYSQLND_MYSQLI);
			mysqlnd_free_result(rs, false);
		}else{
			ctx->rv = php::make_exception(mysqlnd_error(ctx->mysql), mysqlnd_errno(ctx->mysql));
		}
	}
	php::value client::one(php::parameters& params) {
		php::string& table = params[0];
		php::buffer  sql;
		std::memcpy(sql.put(15), "SELECT * FROM `", 15);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		if(params.length() > 1) {
			sql_where(this, params[1], sql);
		}
		if(params.length() > 2) {
			sql_orderby(this, params[2], sql);
		}
		std::memcpy(sql.put(8), " LIMIT 1", 8);
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this, std::move(sql)
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, one_wk, default_cb);
		return flame::async();
	}
	php::value client::select(php::parameters& params) {
		php::string& table = params[0];
		php::buffer  sql;
		std::memcpy(sql.put(7), "SELECT ", 7);
		if(params.length() > 1) {
			if(params[1].is_array()) {
				php::array& col = params[1];
				int j = -1;
				for(auto i=col.begin();i!=col.end();++i) {
					php::string& str = i->second;
					if(++j > 0) sql.add(',');
					sql.add('`');
					std::memcpy(sql.put(str.length()), str.c_str(), str.length());
					sql.add('`');
				}
			}else{
				php::string str = params[1].to_string();
				std::memcpy(sql.put(str.length()), str.c_str(), str.length());
			}
		}else{
			sql.add('*');
		}
		std::memcpy(sql.put(7), " FROM `", 7);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		if(params.length() > 2) {
			sql_where(this, params[2], sql);
		}
		if(params.length() > 3) {
			sql_orderby(this, params[3], sql);
		}
		if(params.length() > 4) {
			sql_limit(this, params[4], sql);
		}
		query_(sql);
		return flame::async();
	}
	void client::autocommit_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		bool on = std::strncmp(ctx->sql.c_str(), "1", 1) == 0;
		mysqlnd_autocommit(ctx->mysql, on);
		ctx->rv = on;
	}
	void client::begin_transaction_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		mysqlnd_autocommit(ctx->mysql, false);
		mysqlnd_begin_transaction(ctx->mysql, TRANS_START_NO_OPT, nullptr);
		ctx->rv = (bool)true;
	}
	void client::commit_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		mysqlnd_commit(ctx->mysql, TRANS_START_NO_OPT, nullptr);
		ctx->rv = (bool)true;
	}
	void client::rollback_wk(uv_work_t* req) {
		mysql_request_t* ctx = reinterpret_cast<mysql_request_t*>(req->data);
		if(!ctx->mysql) {
			ctx->rv = php::make_exception("mysql not connected");
			return;
		}
		mysqlnd_rollback(ctx->mysql, TRANS_START_NO_OPT, nullptr);
		ctx->rv = (bool)true;
	}
	php::value client::autocommit(php::parameters& params) {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this, (params[0].is_true() ? php::string("1",1) : php::string("0", 1))
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, autocommit_wk, default_cb);
		return flame::async();
	}
	php::value client::begin_transaction(php::parameters& params) {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, begin_transaction_wk, default_cb);
		return flame::async();
	}
	php::value client::commit(php::parameters& params) {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, commit_wk, default_cb);
		return flame::async();
	}
	php::value client::rollback(php::parameters& params) {
		mysql_request_t* ctx = new mysql_request_t {
			coroutine::current, mysql_, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, rollback_wk, default_cb);
		return flame::async();
	}
}
}
}
