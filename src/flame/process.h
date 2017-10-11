#pragma once

namespace flame {
	extern enum process_type_t {
		PROCESS_MASTER,
		PROCESS_WORKER,
	} process_type;
	extern std::string    process_name;
	extern uint8_t        process_count;

	class process {
	public:
		static process* prepare();

		virtual void init();
		virtual void run();
	};
	// 当前进程
	extern process* process_self;
}
