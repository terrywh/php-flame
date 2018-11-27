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
		if(options.exists("worker"))
		{
			worker_size = std::min( std::max( (int)options.get("worker").to_integer(), 1), 256 );
			// 使用此环境变量区分父子进程
			if (env.count("FLAME_PROCESS_WORKER") > 0)
			{
				php::callable("cli_set_process_title").call({title + " (flame/" + env["FLAME_PROCESS_WORKER"].to_string() + ")"});
				type = process_type::WORKER;
				worker_.reset(new controller_worker());
				worker_->initialize(options);
			}
			else
			{
				php::callable("cli_set_process_title").call({title + " (flame/m)"});
				type = process_type::MASTER;
				master_.reset(new controller_master());
				master_->initialize(options);
			}
		}
		else
		{
			php::callable("cli_set_process_title").call({title + " (flame/w)"});
			type = process_type::WORKER;
			worker_.reset(new controller_worker());
			worker_->initialize(options);
		}
	}

	void controller::run() {
		if(type == process_type::WORKER)
		{
			worker_->run();
		}
		else
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
