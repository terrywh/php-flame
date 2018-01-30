#pragma once

namespace flame {
	class worker;
	class process {
	public:
		static process* prepare();

		void init();
		void run();

		void worker_start();
		void worker_kill(int sig = SIGINT);
		void on_worker_stop(worker* w);
	private:
		std::set<worker*> workers_;
		uv_signal_t       sigterm_;
		uv_signal_t       sigusr2_;
	};
	extern std::deque<php::callable> quit_cb;
	extern enum process_type_t {
		PROCESS_MASTER,
		PROCESS_WORKER,
	} process_type;
	extern std::string    process_name;
	extern std::uint8_t   process_count;
	// 当前进程
	extern process* process_self;
}
