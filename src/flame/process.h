#pragma once

namespace flame {
	class worker;
	extern std::deque<php::callable> quit_cb;
	extern enum process_type_t {
		PROCESS_MASTER,
		PROCESS_WORKER,
	} process_type;
	extern std::string    process_name;
	extern uint8_t        process_count;

	class process {
	public:
		static process* prepare();

		void init();
		void run();

		void worker_start();
		void worker_stop();
		void on_worker_stop(worker* w);
	private:
		std::set<worker*> workers_;
		uv_signal_t       signal_;

	};
	// 当前进程
	extern process* process_self;
}
