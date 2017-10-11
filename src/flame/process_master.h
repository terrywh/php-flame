#pragma once
#include "process.h"

namespace flame {
	class pipe_worker;
	class process_master: public process {
	public:
		virtual void init() override;
		virtual void run() override;
	private:
		void start_worker();
		void stop_worker(pipe_worker* w);

		std::set<pipe_worker*> workers_;

		friend class pipe_worker;
	};
}
