#pragma once

namespace flame {
	class controller;
	extern std::unique_ptr<controller> controller_;
	class coroutine;
	class controller {
	public:
		enum process_type {
			UNKNOWN = 0,
			MASTER  = 1,
			WORKER  = 2,
		} type;
		boost::asio::io_context  context_ex;
		boost::process::environment environ;
		php::array                  options;
		std::exception_ptr        exception;
		enum {
			STATUS_INITIALIZED = 0x01,
			STATUS_STARTED     = 0x02,
			STATUS_AUTORESTART = 0x04,
			STATUS_SHUTDOWN    = 0x08,
		};
		int                          status;
	private:
		// 主进程
		std::vector<boost::process::child> mworker_;
		boost::process::group                   mg_;
		boost::asio::signal_set                 ms_;
		// 子进程
		std::vector<std::thread>           wworker_;
		boost::asio::signal_set                 ws_;
		// 公共
		int                                  signal_;
		std::list<std::function<void ()>>    before_;
		std::list<std::function<void ()>>    after_;
	public:
		controller();
		controller(const controller& c) = delete;
		void init(const std::string& title);
		void run();
		controller* before(std::function<void ()> fn);
		controller* after(std::function<void ()> fn);
		void shutdown();
	private:
		void spawn(int i);
		void master_run();
		void master_shutdown();
		void worker_run();
		void worker_shutdown();
		void before();
		void after();
	};
}
