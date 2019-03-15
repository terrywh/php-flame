#include "controller.h"
#include "coroutine.h"
#include "controller_master.h"
#include "controller_worker.h"

namespace flame {
	// 全局控制器
	std::unique_ptr<controller> gcontroller;

	controller::controller()
		: type(process_type::UNKNOWN)
		, env(boost::this_process::environment())
		, status(STATUS_UNKNOWN)
		, cbmap(new std::multimap<std::string, php::callable>())
	{

		worker_size = std::atoi(env["FLAME_MAX_WORKERS"].to_string().c_str());
		worker_size = std::min(std::max((int)worker_size, 1), 512);
		// FLAME_MAX_WORKERS 环境变量会被继承, 故此处顺序须先检测子进程
		if (env.count("FLAME_CUR_WORKER") > 0)
		{
			type = process_type::WORKER;
		}
		else if (env.count("FLAME_MAX_WORKERS") > 0)
		{
			type = process_type::MASTER;
		}
		else
		{
			worker_size = 0;
			type = process_type::WORKER;
		}
		mthread_id = std::this_thread::get_id();
	}
	controller *controller::on_init(std::function<void (const php::array &options)> fn)
	{
		init_cb.push_back(fn);
		return this;
	}
	controller* controller::on_stop(std::function<void ()> fn)
	{
		stop_cb.push_back(fn);
		return this;
	}
	void controller::initialize(const std::string& title, const php::array& options) {
		// 多进程模式退出超时时间，默认 1s 超时强制退出
		if(options.exists("timeout")) {
			worker_quit = std::max(static_cast<int>(options.get("timeout")), 64);
		}else{
			worker_quit = 1;
		}
		status |= STATUS_INITIALIZED;
		for (auto fn : init_cb)
		{
			fn(options);
		}
		if(type == process_type::WORKER)
		{
			if (worker_size > 0)
			{
				std::string index = env["FLAME_CUR_WORKER"].to_string();
				php::callable("cli_set_process_title").call({title + " (php-flame/" + index + ")"});
			}else{
				php::callable("cli_set_process_title").call({title + " (php-flame/w)"});
			}
			worker_.reset(new controller_worker());
			worker_->initialize(options);
		}else/* if(type == process_type::MASTER)*/
		{
			php::callable("cli_set_process_title").call({title + " (php-flame/m)"});
			master_.reset(new controller_master());
			master_->initialize(options);
		}
	}

	void controller::run() {
		status &= controller_status::STATUS_RUN;
		if(type == process_type::WORKER)
		{
			worker_->run();
		}
		else /* if(type == process_type::MASTER)*/
		{
			master_->run();
		}
		delete cbmap;
		// 运行完毕
		if(status & controller_status::STATUS_EXCEPTION) {
			exit(-1);
		} else {
			exit(0);
		}
		// 由于 PHP 自行回收可能导致 C++ 空间中的 PHP 对象被进行二次释放导致异常
	}

	void controller::stop()
	{
		for (auto fn : stop_cb)
		{
			fn();
		}
	}
}
