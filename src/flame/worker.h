#pragma once

namespace flame {
	class process;
	class worker {
	public:
		worker(process* master);
		void start();
		void kill();
		void stop();
	private:
		process*         master_;
		uv_process_t     proc_;
		uv_timer_t       timer_;
		static void on_worker_exit(uv_process_t* proc, int64_t exit_status, int term_signal);
		static void on_worker_restart(uv_timer_t* tm);
	};
}
