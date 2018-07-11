#include "time/time.h"
#include "controller.h"
#include "coroutine.h"
#include "execinfo.h"

namespace flame {
	std::unique_ptr<controller> controller_;
	controller::controller()
	: type(UNKNOWN)
	, environ(boost::this_process::environment())
	, status(0) {
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
			worker_.resize(count);
			for(int i=0; i<worker_.size(); ++i) {
				spawn(i);
			}
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
		worker_[i] = boost::process::child(executable, script, env, g_, context,
			boost::process::std_out > stdout,
			boost::process::std_err > stdout,
			boost::process::on_exit = [this, i] (int exit_code, const std::error_code&) {
			if(exit_code != 0 && (status & STATUS_AUTORESTART)) {
				std::clog << "[" << time::datetime() << "] (WARN) worker unexpected exit, restart in 3s ..." << std::endl;
				auto tm = std::make_shared<boost::asio::steady_timer>(context, std::chrono::seconds(3));
				tm->async_wait([this, tm, i] (const boost::system::error_code& error) {
					if(!error) spawn(i);
				});
			}else{
				for(int i=0;i<worker_.size();++i) {
					if(worker_[i].running()) return;
				}
				context.stop();
			}
		});
	}
	void controller::master_run() {
		before();
		// 主进程负责进程监控, 转发信号
		boost::asio::signal_set s(context, SIGINT, SIGTERM);
		s.async_wait([this] (const boost::system::error_code& error, int sig) {
			for(int i=0; i<worker_.size(); ++i) {
				::kill(worker_[i].id(), sig);
			}
			context.stop();
		});
		context.run();
		after();
		// 这里有两种情况:
		// 1. 所有子进程自主退出;
		// 2. 子进程还未退出, 一段时间后强制结束;
		std::error_code error;
		if(!g_.wait_for(std::chrono::seconds(10), error)) {
			g_.terminate(); // 10s 后还未结束, 强制杀死
		}
	}
	void controller::worker_run() {
		before();
		// boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(context_ex.get_executor());
		boost::asio::signal_set s(context_ex, SIGINT, SIGTERM);
		int signal = 0;
		s.async_wait([this, &signal] (const boost::system::error_code& error, int sig) {
			signal = sig;
			context.stop();
		});
		// 辅助工作线程
		std::thread workers[4];
		for(int i=0;i<4;++i) {
			workers[i] = std::thread([this] {
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
		s.cancel();
		for(int i=0;i<4;++i) {
			if(workers[i].joinable()) {
				workers[i].join();
			}
		}
		after();
		// 停止所有还在运行的协程
		coroutine::shutdown();
		// !!! 发生异常退出, 防止 PHP 引擎将还存活的对象内存提前释放
		if(exception) exit(-1);
		if(signal == SIGINT) exit(-2);
	}
}