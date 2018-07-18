#include "../controller.h"
#include "../time/time.h"
#include "logger.h"
#include "log.h"
#include "../core.h"

namespace flame {
namespace log {
	php::value logger_ref;
	logger*    logger_ = nullptr;

	static void write_before_exit(const std::exception& ex) {
		static boost::format fmt(" %3% [%1%] (FAIL) Uncaught C++ Exception: %2%");
		if(logger_) {
			std::string log = logger_->write(fmt % time::datetime() % ex.what());
			if(logger_->to_file()) {
				std::clog << log.substr(33) << std::endl;
			}
		}
	}
	static void write_before_exit(const php::exception& ex) {
		static boost::format fmt(" %3% [%1%] (FAIL) Uncaught PHP Exception: %2%");
		// PHP 异常可以获得更多的信息
		php::object obj(ex);
		php::string str = obj.call("__tostring");
		if(logger_) {
			std::string log = logger_->write(fmt % time::datetime() % str);
			if(logger_->to_file()) {
				std::clog << log.substr(33) << std::endl;
			}
		}
	}

	void declare(php::extension_entry& ext) {
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			// 在框架初始化后创建全局日志对象
			controller_->before([] () {
				php::string file;
				if(controller_->options.exists("logger")) {
					file = controller_->options.get("logger");
				}
				logger_ref = php::object(php::class_entry<logger>::entry(), {file});
				logger_    = static_cast<logger*>(php::native(logger_ref));
			})->after([] () {
				if(controller_ && controller_->exception) {
					try{
						std::rethrow_exception(controller_->exception);
					}catch(const php::exception& ex) {
						write_before_exit(ex);
					}catch(const std::exception& ex) {
						write_before_exit(ex);
					}catch(...) {
						assert(0 && "未知的错误");
					}
				}
				// 一定要提前销毁
				logger_ref = nullptr;
				logger_ = nullptr;
			});
			return true;
		});
		ext
			.function<rotate>("flame\\log\\rotate")
			.function<write>("flame\\log\\write")
			.function<info>("flame\\log\\info")
			.function<warn>("flame\\log\\warn")
			.function<fail>("flame\\log\\fail");

		logger::declare(ext);
	}
	static void init_guard() {
		if(!logger_) {
			throw php::exception(zend_ce_parse_error, "flame not initialized");
		}
	}
	php::value rotate(php::parameters& params) {
		init_guard();
		return logger_->rotate(params);
	}
	php::value write(php::parameters& params) {
		init_guard();
		return logger_->write(params);
	}
	php::value info(php::parameters& params) {
		init_guard();
		return logger_->info(params);
	}
	php::value warn(php::parameters& params) {
		init_guard();
		return logger_->warn(params);
	}
	php::value fail(php::parameters& params) {
		init_guard();
		return logger_->fail(params);
	}

}
}
