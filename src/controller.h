#pragma once
#include "vendor.h"

namespace flame {
	class controller_master;
	class controller_worker;
	class controller {
	public:
		boost::asio::io_context context_x;
		boost::asio::io_context context_y;
		zend_execute_data* default_execute_data;
		enum class process_type
		{
			UNKNOWN = 0,
			MASTER = 1,
			WORKER = 2,
		} type;
		boost::process::environment env;
		enum controller_status
		{
			STATUS_UNKNOWN = 0,
			STATUS_INITIALIZED = 1,
			STATUS_SHUTDOWN = 2,
			STATUS_EXCEPTION = 4,
		};
		int status;
		std::size_t    worker_size;
		std::thread::id mthread_id;
		// 防止 PHP 提前回收, 使用堆容器
		std::multimap<std::string, php::callable>* cbmap;
	private:
		// 核心进程对象
		std::unique_ptr<controller_master> master_;
		// 工作进程对象
		std::unique_ptr<controller_worker> worker_;
		// 公共
		std::list<std::function<void (const php::array& options)>>  init_cb;
		std::list<std::function<void ()>>  stop_cb;
	public:
		controller();
		controller(const controller& c) = delete;
		void initialize(const std::string& title, const php::array &options);
		void run();
		controller *on_init(std::function<void(const php::array &options)> fn);
		controller* on_stop(std::function<void ()> fn);
		void stop();
	};

	extern std::unique_ptr<controller> gcontroller;
}
