#include "vendor.h"
#include "core.h"
#include "task_runner.h"

task_wrapper::task_wrapper(const std::function<void(php::callable)>& t, const php::callable& d)
: task(t), done(d), work(core::io()) {

}

task_runner::task_runner()
: stopped_(false) {
	for(int i=0;i<thread_.size();++i) {
		thread_[i] = std::thread(run, this, i);
	}
}
void task_runner::run(task_runner* self, int index) {
	task_wrapper* tr = nullptr;
NEXT_TASK:
	if(!self->queue_.pop(tr)) {
		if(self->stopped_) {
			return;
		}
		std::fprintf(stderr, ".\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		goto NEXT_TASK;
	}
	tr->task(php::value([tr] (php::parameters& params) mutable -> php::value {
		// 对 tr->done 的调用过程需要恢复到 io_service 主线程
		if(params.length() > 1) {
			php::value ex = params[0], rv = params[1];
			core::io().post([tr, ex, rv] () mutable {
				tr->done(ex, rv);
				delete tr;
			});
		}else if(params.length() > 0) {
			php::value ex = params[0];
			core::io().post([tr, ex] () mutable {
				tr->done(ex);
				delete tr;
			});
		}else{
			core::io().post([tr] () mutable {
				tr->done();
				delete tr;
			});
		}
		return nullptr;
	}), i);
	// !!! 这里有个潜藏的问题，用户有可能不掉用上面定义的回调函数（导致内存泄漏）
	if(!self->stopped_) {
		goto NEXT_TASK;
	}
	std::fprintf(stderr, "x\n");
}

void task_runner::stop_wait() {
	stopped_ = true;
	for(int i=0;i<thread_.size();++i) {
		if(thread_[i].joinable()) {
			thread_[i].join();
		}
	}
}

php::value task_runner::async(const std::function<void( php::callable )>& task) {
	return php::value([this, task] (php::parameters& params) mutable -> php::value {
		php::callable done = params[0];
		queue_.push(new task_wrapper(task, done));
		return nullptr;
	});

}
