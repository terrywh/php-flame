#pragma once

namespace flame {
	class process_master;
	class pipe_worker {
	public:
		pipe_worker(process_master* master);
		void start();
	private:
		process_master*  master_;
		uv_process_t     proc_;
		uv_pipe_t        pipe_;
		uv_timer_t       timer_;
		static void on_worker_exit(uv_process_t* proc, int64_t exit_status, int term_signal);
		static void on_worker_restart(uv_timer_t* tm);
	};
}
