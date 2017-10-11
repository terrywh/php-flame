#include "process.h"
#include "process_master.h"
#include "process_worker.h"
#include "coroutine.h"

namespace flame {
	process_type_t process_type;
	std::string    process_name;
	uint8_t        process_count;
	process*       process_self;
	uv_loop_t*     loop;
	process* process::prepare() {
		// 确认当前是父进程还是子进程
		char* worker = std::getenv("FLAME_CLUSTER_WORKER");
		if(worker == nullptr) {
			process_type = PROCESS_MASTER;
			process_self = new process_master();
		}else{
			process_type = PROCESS_WORKER;
			process_self = new process_worker();
		}
		return process_self;
	}

	void process::init() {

	}

	void process::run() {
		uv_run(flame::loop, UV_RUN_DEFAULT);
	}
}
