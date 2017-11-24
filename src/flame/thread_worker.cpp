#include "thread_worker.h"
#include "coroutine.h"

namespace flame {
	thread_worker::thread_worker()
	: exit_(false) {
		uv_async_init(flame::loop, &async_master, master_cb);
		async_master.data = this;
		uv_unref((uv_handle_t*)&async_master);
		uv_mutex_init(&mutex_master);

		uv_loop_init(&loop);
		uv_async_init(&loop, &async_worker, worker_cb);
		async_worker.data = this;
		uv_mutex_init(&mutex_worker);

		uv_thread_create(&tid_, run, this);
	}
	thread_worker::~thread_worker() {
		uv_stop(&loop);
	}
	void thread_worker::join() {
		if(!exit_) {
			exit_ = true;
			uv_async_send(&async_worker);
			uv_thread_join(&tid_);
		}
	}
	void thread_worker::run(void* data) {
		reinterpret_cast<thread_worker*>(data)->run();
	}
	void thread_worker::run() {
		uv_run(&loop, UV_RUN_DEFAULT);
	}
	void thread_worker::worker_cb(uv_async_t* handle) {
		reinterpret_cast<thread_worker*>(handle->data)->worker_cb();
	}
	void thread_worker::worker_cb() {
		while(!exit_) {
			uv_mutex_lock(&mutex_worker);
			if(queue_worker.empty()) {
				uv_mutex_unlock(&mutex_worker);
				break;
			}
			uv_work_t* req = queue_worker.front();
			queue_worker.pop_front();
			uv_mutex_unlock(&mutex_worker);

			req->work_cb(req);

			if(req->after_work_cb) {
				uv_mutex_lock(&mutex_master);
				queue_master.push_back(req);
				uv_mutex_unlock(&mutex_master);

				uv_async_send(&async_master);
			}
		}
		// 退出结束线程循环
		if(exit_) uv_stop(&loop);
	}
	void thread_worker::master_cb(uv_async_t* handle) {
		reinterpret_cast<thread_worker*>(handle->data)->master_cb();
	}
	void thread_worker::master_cb() {
		while(!exit_) {
			uv_mutex_lock(&mutex_master);
			if(queue_master.empty()) {
				uv_mutex_unlock(&mutex_master);
				break;
			}
			uv_work_t* req = queue_master.front();
			queue_master.pop_front();
			uv_mutex_unlock(&mutex_master);

			req->after_work_cb(req, 0);
		}
	}
	void thread_worker::queue_work(uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb) {
		if(exit_) return;

		req->work_cb = work_cb;
		req->after_work_cb = after_work_cb;

		uv_mutex_lock(&mutex_worker);
		queue_worker.push_back(req);
		uv_mutex_unlock(&mutex_worker);

		uv_async_send(&async_worker);
	}
}
