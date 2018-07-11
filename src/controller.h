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
		};
		int                          status;
	private:
		std::vector<boost::process::child> worker_;
		boost::process::group                   g_;
		
	
		std::list<std::function<void ()>>    before_;
		std::list<std::function<void ()>>    after_;
	public:
		controller();
		controller(const controller& c) = delete;
		void init(const std::string& title);
		void run();
		controller* before(std::function<void ()> fn);
		controller* after(std::function<void ()> fn);
		
	private:
		void spawn(int i);
		void master_run();
		void worker_run();
		void before();
		void after();
	};
}