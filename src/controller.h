#pragma once
#include "vendor.h"

namespace flame {
	class controller_master {
	public:
		controller_master();
		void run();

	  private:
		std::vector<boost::process::child> worker_;
		boost::process::group               group_;
		boost::asio::signal_set            signal_;

	};
	class controller_worker {
	public:
		controller_worker();
		void run();

	  private:
		std::vector<std::thread>           thread_;
		boost::asio::signal_set            signal_;

		
	};
	class controller {
	public:
	  boost::asio::io_context context_x;
	  boost::asio::io_context context_y;

	  enum class process_type
	  {
		  UNKNOWN = 0,
		  MASTER = 1,
		  WORKER = 2,
	  } type;
	  boost::process::environment env;
	  boost::program_options::variables_map options;
	  enum class controller_status
	  {
		  UNKNOWN = 0,
	  } status;
	private:
		// 核心进程对象
		std::unique_ptr<controller_master> master_;
		// 工作进程对象
		std::unique_ptr<controller_worker> worker_;
		
		// 公共
		std::list<std::function<void ()>>  init_cb;
		std::list<std::function<void ()>>  stop_cb;
	public:
		controller();
		controller(const controller& c) = delete;
		void initialize();
		void run();
		controller* on_init(std::function<void ()> fn);
		controller* on_stop(std::function<void ()> fn);
	};

	extern std::unique_ptr<controller> gcontroller;
}
