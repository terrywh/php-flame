#include "time/time.h"
#include "log/log.h"
#include "log/logger.h"
#include "controller.h"
#include "coroutine.h"
// #include "execinfo.h"

namespace flame {
	std::unique_ptr<controller> controller_;
	controller::controller()
	: type(UNKNOWN)
	, environ(boost::this_process::environment())
	, status(0)
	, ms_(context, SIGINT, SIGTERM)
	, ws_(context_ex, SIGINT, SIGTERM) {
		if(environ.count("FLAME_PROCESS_WORKER") == 0) {
			type = MASTER;
		}else{
			type = WORKER;
		}
	}
	void controller::init(const std::string& title) {
		environ["FLAME_PROCESS_TITLE"] = title;
		if(type == MASTER) {
			php::callable("cli_set_process_title").call({ title + " (flame/m)" });
			int count = 1;
			if(options.exists("worker")) {
				count = std::min(256, std::max((int)options.get("worker").to_integer(), 1));
			}
			mworker_.resize(count);
		}else{
			php::callable("cli_set_process_title").call({ title + " (flame/" + environ["FLAME_PROCESS_WORKER"].to_string() + ")" });
		}
		php::value debug = options.get("debug");
		if(!debug.typeof(php::TYPE::UNDEFINED) && debug.empty()) {
			status |= STATUS_AUTORESTART;
		}
		status |= STATUS_INITIALIZED;
	}
	controller* controller::before(std::function<void ()> fn) {
		before_.push_back(fn);
		return this;
	}
	controller* controller::after(std::function<void ()> fn) {
		after_.push_back(fn);
		return this;
	}
	void controller::before() {
		for(auto fn: before_) {
			fn();
		}
	}
	void controller::after() {
		for(auto fn: after_) {
			fn();
		}
	}
	void controller::run() {
		status |= STATUS_STARTED;
		switch(type) {
		case MASTER:
			master_run();
			break;
		case WORKER:
			worker_run();
			break;
		default:
			assert(0 && "未知进程类型");
		}
	}
	void controller::spawn(int i) {
		std::string executable = php::constant("PHP_BINARY");
		std::string script = php::server("SCRIPT_FILENAME");
		boost::process::environment env = environ;
		env["FLAME_PROCESS_WORKER"] = std::to_string(i+1);
		mworker_[i] = boost::process::child(executable, script, env, mg_, context,
			boost::process::std_out > stdout,
			boost::process::std_err > stdout,
			boost::process::on_exit = [this, i] (int exit_code, const std::error_code&) {
			if(exit_code != 0 && (status & STATUS_AUTORESTART)) {
				// 日志记录
				log::logger_->write(boost::format(" %2% [%1%] (WARN) worker unexpected exit, restart in 3s ...") % time::datetime());
				auto tm = std::make_shared<boost::asio::steady_timer>(context, std::chrono::seconds(3));
				tm->async_wait([this, tm, i] (const boost::system::error_code& error) {
					if(!error) spawn(i);
				});
			}else{
				for(int i=0;i<mworker_.size();++i) {
					if(mworker_[i].running()) return;
				}
				context.stop();
			}
		});
	}
	void controller::master_run() {
		before();
		for(int i=0; i<mworker_.size(); ++i) {
			spawn(i);
		}
		// 主进程负责进程监控, 转发信号
		ms_.async_wait([this] (const boost::system::error_code& error, int sig) {
			// 日志记录
			log::logger_->write(boost::format(" %3% [%1%] (INFO) master receives signal '%2%', exiting ...") % time::datetime() % sig);
			signal_ = sig;
			for(int i=0; i<mworker_.size(); ++i) {
				::kill(mworker_[i].id(), sig);
			}
			context.stop();
		});
		context.run();
		master_shutdown();
	}
	void controller::worker_run() {
		before();
		// boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(context_ex.get_executor());
		ws_.async_wait([this] (const boost::system::error_code& error, int sig) {
			signal_ = sig;
			context.stop();
		});
		// 辅助工作线程
		wworker_.resize(4);
		for(int i=0;i<4;++i) {
			wworker_[i] = std::thread([this] {
				context_ex.run();
			});
		}
		// 工作进程负责执行
		try{
			context.run();

			php::exception::rethrow();
		}catch(...) {
			exception = std::current_exception();
		}
		worker_shutdown();
	}
	void controller::shutdown() {
		switch(type) {
		case MASTER:
			master_shutdown();
			break;
		case WORKER:
			worker_shutdown();
			break;
		default:
			assert(0 && "未知进程类型");
		}
	}
	void controller::master_shutdown() {
		if(status & STATUS_SHUTDOWN) return;
		status |= STATUS_SHUTDOWN;

		options = nullptr;
		context.stop();
		context_ex.stop();
		// 这里有两种情况:
		// 1. 所有子进程自主退出;
		// 2. 子进程还未退出, 一段时间后强制结束;
		std::error_code error;
		if(!mg_.wait_for(std::chrono::seconds(10), error)) {
			mg_.terminate(); // 10s 后还未结束, 强制杀死
		}
		after();
	}
	void controller::worker_shutdown() {
		if(status & STATUS_SHUTDOWN) return;
		status |= STATUS_SHUTDOWN;

		options = nullptr;
		context.stop();
		context_ex.stop();
		ws_.cancel();
		for(int i=0;i<4;++i) {
			if(wworker_[i].joinable()) {
				wworker_[i].join();
			}
		}
		after();
		// 停止所有还在运行的协程
		coroutine::shutdown();
		// !!! 发生异常退出, 防止 PHP 引擎将还存活的对象内存提前释放
		if(exception) exit(-1);
		if(signal_ == SIGINT) exit(-2);
	}

}
