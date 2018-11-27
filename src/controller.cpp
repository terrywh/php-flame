#include "controller.h"
#include "coroutine.h"

namespace flame {
// 全局控制器
std::unique_ptr<controller> gcontroller;

controller::controller()
	: type(process_type::UNKNOWN)
	, env(boost::this_process::environment())
	, status(controller_status::UNKNOWN) {

	mthread_id = std::this_thread::get_id();
}

controller* controller::on_init(std::function<void ()> fn)
{
	init_cb.push_back(fn);
	return this;
}
controller* controller::on_stop(std::function<void ()> fn)
{
	stop_cb.push_back(fn);
	return this;
}

void controller::initialize() {
	// 使用此环境变量区分父子进程
	// if (env.count("FLAME_PROCESS_WORKER") > 0)
	// {
		type = process_type::WORKER;
		worker_.reset(new controller_worker());
	// }
	// else
	// {
	// 	type = process_type::MASTER;
	// 	master_.reset(new controller_master());
	// }
	for (auto fn : init_cb) {
		fn();
	}
}

void controller::run() {
	// if(type == process_type::WORKER) {
	worker_->run();
	// }else{
		// master_->run();
	// }
	for (auto fn : stop_cb)
	{
		fn();
	}
}

controller_master::controller_master()
	: signal_(gcontroller->context_x/*, SIGINT, SIGTERM*/)
{
}

void controller_master::run() {
	// 主进程的启动过程:
	// 1. 新增环境变量及命令行参数, 启动子进程;
	// 2. 侦听子进程退出动作，按状态标志进行拉起；
	// 3. 等待所有子进程输出, 并进行日志数据重定向(日志配置);
	// 4. 监听信号进行日志重载或停止
	// signal_.async_wait([this](const boost::system::error_code &error, int signal) {

	// });
	// 5. 启动 context_x 运行
	gcontroller->context_x.run();
}

controller_worker::controller_worker()
	: signal_(gcontroller->context_x/*, SIGINT, SIGTERM*/)
{
	
}

void controller_worker::run() {
	auto work = boost::asio::make_work_guard(gcontroller->context_y);
	// 子进程的启动过程:
	// 1. 监听信号进行日志重载或停止
	// signal_.async_wait([this] (const boost::system::error_code& error, int signal) {
		
	// });
	// 2. 启动线程池, 并使用线程池运行 context_y
	thread_.resize(3);
	for(int i=0;i<thread_.size();++i) {
		thread_[i] = std::thread([this] {
			gcontroller->context_y.run();
		});
	}
	// 3. 启动 context_x 运行
	gcontroller->context_x.run();
	work.reset();
	for (int i=0; i<thread_.size(); ++i)
	{
		thread_[i].join();
	}
	
}


	// void controller::init(const std::string& title) {
	// 	environ["FLAME_PROCESS_TITLE"] = title;
	// 	if(type == MASTER) {
	// 		php::callable("cli_set_process_title").call({ title + " (flame/m)" });
	// 		int count = 1;
	// 		if(options.exists("worker")) {
	// 			count = std::min(256, std::max((int)options.get("worker").to_integer(), 1));
	// 		}
	// 		mworker_.resize(count);
	// 	}else{
	// 		php::callable("cli_set_process_title").call({ title + " (flame/" + environ["FLAME_PROCESS_WORKER"].to_string() + ")" });
	// 	}
	// 	php::value debug = options.get("debug");
	// 	if(!debug.typeof(php::TYPE::UNDEFINED) && debug.empty()) {
	// 		status |= STATUS_AUTORESTART;
	// 	}
	// 	status |= STATUS_INITIALIZED;
	// }
	// controller* controller::before(std::function<void ()> fn) {
	// 	before_.push_back(fn);
	// 	return this;
	// }
	// controller* controller::after(std::function<void ()> fn) {
	// 	after_.push_back(fn);
	// 	return this;
	// }
	// void controller::before() {
	// 	for(auto fn: before_) {
	// 		fn();
	// 	}
	// }
	// void controller::after() {
	// 	for(auto fn: after_) {
	// 		fn();
	// 	}
	// }
	// void controller::run() {
	// 	status |= STATUS_STARTED;
	// 	switch(type) {
	// 	case MASTER:
	// 		master_run();
	// 		break;
	// 	case WORKER:
	// 		worker_run();
	// 		break;
	// 	default:
	// 		assert(0 && "未知进程类型");
	// 	}
	// }
	// static std::string build_command_line() {
	// 	php::stream_buffer sb;
	// 	std::ostream os(&sb);
	// 	os << php::constant("PHP_BINARY");
	// 	php::array argv = php::server("argv");
	// 	for(auto i=argv.begin(); i!=argv.end(); ++i) {
	// 		os << " " << i->second;
	// 	}
	// 	return std::string(sb.data(), sb.size());
	// }
	// void controller::spawn(int i) {
	// 	std::string cmd = build_command_line();

	// 	boost::process::environment env = environ;
	// 	env["FLAME_PROCESS_WORKER"] = std::to_string(i+1);
	// 	mworker_[i] = boost::process::child(cmd, env, mg_, context,
	// 		boost::process::std_out > stdout,
	// 		boost::process::std_err > stdout,
	// 		boost::process::on_exit = [this, i] (int exit_code, const std::error_code& error) {
	// 			if(error.value() == static_cast<int>(std::errc::no_child_process)) return;
	// 			if(exit_code != 0 && (status & STATUS_AUTORESTART)) {
	// 				// 日志记录
	// 				log::logger_->write(boost::format(" %2% [%1%] (WARN) worker unexpected exit, restart in 3s ...") % time::datetime());
	// 				auto tm = std::make_shared<boost::asio::steady_timer>(context, std::chrono::seconds(3));
	// 				tm->async_wait([this, tm, i] (const boost::system::error_code& error) {
	// 					if(!error) spawn(i);
	// 				});
	// 			}else{
	// 				for(int i=0;i<mworker_.size();++i) {
	// 					if(mworker_[i].running()) return;
	// 				}
	// 				context.stop();
	// 			}
	// 		});
	// }
	// void controller::master_run() {
	// 	before();
	// 	for(int i=0; i<mworker_.size(); ++i) {
	// 		spawn(i);
	// 	}
	// 	// 主进程负责进程监控, 转发信号
	// 	ms_.async_wait([this] (const boost::system::error_code& error, int sig) {
	// 		// 日志记录
	// 		log::logger_->write(boost::format(" %3% [%1%] (INFO) master receives signal '%2%', exiting ...") % time::datetime() % sig);
	// 		signal_ = sig;
	// 		for(int i=0; i<mworker_.size(); ++i) {
	// 			::kill(mworker_[i].id(), sig);
	// 		}
	// 		context.stop();
	// 	});
	// 	context.run();
	// 	master_shutdown();
	// }
	// void controller::worker_run() {
	// 	before();
	// 	// boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(context_ex.get_executor());
	// 	ws_.async_wait([this] (const boost::system::error_code& error, int sig) {
	// 		signal_ = sig;
	// 		context.stop();
	// 	});
	// 	// 辅助工作线程
	// 	wworker_.resize(4);
	// 	for(int i=0;i<4;++i) {
	// 		wworker_[i] = std::thread([this] {
	// 			context_ex.run();
	// 		});
	// 	}
	// 	// 工作进程负责执行
	// 	try{
	// 		context.run();

	// 		php::exception::rethrow();
	// 	}catch(...) {
	// 		exception = std::current_exception();
	// 	}
	// 	worker_shutdown();
	// }
	// void controller::shutdown() {
	// 	switch(type) {
	// 	case MASTER:
	// 		master_shutdown();
	// 		break;
	// 	case WORKER:
	// 		worker_shutdown();
	// 		break;
	// 	default:
	// 		assert(0 && "未知进程类型");
	// 	}
	// }
	// void controller::master_shutdown() {
	// 	if(status & STATUS_SHUTDOWN) return;
	// 	status |= STATUS_SHUTDOWN;

	// 	options = nullptr;
	// 	context.stop();
	// 	context_ex.stop();
	// 	// 这里有两种情况:
	// 	// 1. 所有子进程自主退出;
	// 	// 2. 子进程还未退出, 一段时间后强制结束;
	// 	std::error_code error;
	// 	if(!mg_.wait_for(std::chrono::seconds(10), error)) {
	// 		mg_.terminate(); // 10s 后还未结束, 强制杀死
	// 	}
	// 	after();
	// 	exit(0);
	// }
	// void controller::worker_shutdown() {
	// 	if(status & STATUS_SHUTDOWN) return;
	// 	status |= STATUS_SHUTDOWN;

	// 	options = nullptr;
	// 	context.stop();
	// 	context_ex.stop();
	// 	ws_.cancel();
	// 	for(int i=0;i<wworker_.size();++i) {
	// 		if(wworker_[i].joinable()) {
	// 			wworker_[i].join();
	// 		}
	// 	}
	// 	after();
	// 	// 停止所有还在运行的协程
	// 	coroutine::shutdown();
	// 	// !!! 发生异常退出, 防止 PHP 引擎将还存活的对象内存提前释放
	// 	if(exception) exit(-99);
	// 	if(signal_ == SIGINT) exit(0);
	// 	exit(0);
	// }

}
