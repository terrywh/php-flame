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
		, status(STATUS_UNKNOWN) {
		// FLAME_MAX_WORKERS 环境变量会被继承, 故此处顺序须先检测子进程
		if (env.count("FLAME_CUR_WORKER") > 0)
		{
			type = process_type::WORKER;
		}
		else if (env.count("FLAME_MAX_WORKERS") > 0)
		{
			worker_size = std::atoi(env["FLAME_MAX_WORKERS"].to_string().c_str());
			worker_size = std::min(std::max((int)worker_size, 1), 512);
			type = process_type::MASTER;
		} 
		else
		{
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
		status |= STATUS_INITIALIZED;
		for (auto fn : init_cb)
		{
			fn(options);
		}
		if(type == process_type::WORKER)
		{
			std::string index = env["FLAME_CUR_WORKER"].to_string();
			if(index.empty()) index = "w";
			php::callable("cli_set_process_title").call({title + " (php-flame/" + index + ")"});
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
		if(type == process_type::WORKER)
		{
			worker_->run();
		}
		else /* if(type == process_type::MASTER)*/
		{
			master_->run();
		}
		// 运行完毕
		for (auto fn : stop_cb)
		{
			fn();
		}
	}
}
