#pragma once
#include "process.h"

namespace flame {
	class pipe_worker;
	class process_master: public process {
	public:
		virtual void init() override;
		virtual void run() override;
		void send_to_worker_one(std::string msg, uv_stream_t* ss);
		void send_to_worker_all(std::string msg);
	private:
		void start_worker();
		void stop_worker(pipe_worker* w);

		std::set<pipe_worker*>           worker_all;
		std::set<pipe_worker*>::iterator worker_i;

		friend class pipe_worker;
	};
}
