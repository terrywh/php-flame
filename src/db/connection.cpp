#include "../vendor.h"
#include "connection.h"
#include "../core.h"

namespace db {
	static connection* conn = nullptr;
	void connection::init(php::extension_entry& extension) {
		extension.on_module_startup([] (php::extension_entry& extension) -> bool {
			conn = new connection();
			return true;
		});
		extension.on_module_shutdown([] (php::extension_entry& extension) -> bool {
			delete conn;
			return true;
		});
		php::class_entry<connection> class_db_connection("flame\\db\\connection");
		class_db_connection.add<connection::preserve>("preserve");
	}
	connection::~connection() {
		for(auto i=timer_.begin();i!=timer_.end();++i) {
			delete (*i);
		}
	}
	php::value connection::preserve(php::parameters& params) {
		php::object& obj = params[0];
		int itv = 300; // interval 间隔
		if(params.length() > 1) {
			itv = params[1];
		}
		std::string fn = "ping"; // 调用指定方法
		if(params.length() > 2) {
			fn.assign(params[2]);
		}
		boost::asio::steady_timer* tmr = new boost::asio::steady_timer(core::io());
		conn->timer_.push_back(tmr);
		await_timer(tmr, obj, itv, fn);
	}
	void connection::await_timer(boost::asio::steady_timer* tmr, php::object& obj, int itv, const std::string& fn) {
		tmr->expires_from_now(std::chrono::seconds(itv));
		tmr->async_wait([tmr, obj, itv, fn] (const boost::system::error_code& err) mutable {
			if(!err) {
				obj.call(fn);
				await_timer(tmr, obj, itv, fn);
			}else{
				// TODO 做出提示，清理 timer 对象？
			}
		});
	}
}
