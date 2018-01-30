#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "thread_worker.h"

namespace flame {
	thread_worker::thread_worker() {
		exit_req.data = nullptr;

		uv_async_init(flame::loop, &async_master, master_cb);
		async_master.data = this;
		uv_mutex_init(&mutex_master);

		uv_loop_init(&loop);
		uv_async_init(&loop, &async_worker, worker_cb);
		async_worker.data = this;
		uv_mutex_init(&mutex_worker);

		uv_thread_create(&tid_, run, this);
	}
	uv_mutex_t* thread_worker::worker_lock() {
		return &mutex_worker;
	}
	void thread_worker::close_work(void *data, uv_work_cb work_cb, uv_after_work_cb after_work_cb) {
		if(exit_req.data == nullptr) {
			exit_req.data = data;
			exit_req.work_cb = work_cb;
			exit_req.after_work_cb = after_work_cb;

			uv_mutex_lock(&mutex_worker);
			queue_worker.push_back(&exit_req);
			uv_mutex_unlock(&mutex_worker);

			uv_async_send(&async_worker);
			uv_thread_join(&tid_);
		}
	}
	void thread_worker::run(void* data) {
		uv_run(&reinterpret_cast<thread_worker*>(data)->loop, UV_RUN_DEFAULT);
	}
	void thread_worker::worker_cb(uv_async_t* handle) {
		thread_worker* self = reinterpret_cast<thread_worker*>(handle->data);
		while(true) {
			uv_mutex_lock(&self->mutex_worker);
			if(self->queue_worker.empty()) {
				uv_mutex_unlock(&self->mutex_worker);
				break;
			}
			uv_work_t* req = self->queue_worker.front();
			self->queue_worker.pop_front();
			uv_mutex_unlock(&self->mutex_worker);
			// 退出时的特殊实现
			if(req == &self->exit_req) {
				assert(self->queue_worker.empty());
				uv_close((uv_handle_t*)&self->async_worker, worker_close_cb);
			}else{ // 正常状态工作线程任务
				if(req->work_cb) req->work_cb(req);
				// 主线程任务
				uv_mutex_lock(&self->mutex_master);
				self->queue_master.push_back(req);
				uv_mutex_unlock(&self->mutex_master);
				uv_async_send(&self->async_master);
			}
			
		}
	}
	void thread_worker::master_cb(uv_async_t* handle) {
		thread_worker* self = reinterpret_cast<thread_worker*>(handle->data);
		while(true) {
			uv_mutex_lock(&self->mutex_master);
			if(self->queue_master.empty()) {
				uv_mutex_unlock(&self->mutex_master);
				break;
			}
			uv_work_t* req = self->queue_master.front();
			self->queue_master.pop_front();
			uv_mutex_unlock(&self->mutex_master);
			// 退出时的特殊实现
			if(req == &self->exit_req) {
				assert(self->queue_master.empty());
				uv_close((uv_handle_t*)&self->async_master, master_close_cb);
			}else if(req->after_work_cb) { // 正常状态主线程任务
				req->after_work_cb(req, 0);
			}
		}
	}
	void thread_worker::queue_work(uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb) {
		if(exit_req.data != nullptr) {
			throw php::exception("cannot queue work after closing");
		}
		req->work_cb = work_cb;
		req->after_work_cb = after_work_cb;

		uv_mutex_lock(&mutex_worker);
		queue_worker.push_back(req);
		uv_mutex_unlock(&mutex_worker);

		uv_async_send(&async_worker);
	}
	void thread_worker::worker_close_cb(uv_handle_t* handle) {
		thread_worker* self = reinterpret_cast<thread_worker*>(handle->data);
		if(self->exit_req.work_cb) {
			self->exit_req.work_cb(&self->exit_req);
		}
		uv_loop_close(&self->loop);
		// 主线程任务
		uv_mutex_lock(&self->mutex_master);
		self->queue_master.push_back(&self->exit_req);
		uv_mutex_unlock(&self->mutex_master);
		uv_async_send(&self->async_master);
	}
	void thread_worker::master_close_cb(uv_handle_t* handle) {
		thread_worker* self = reinterpret_cast<thread_worker*>(handle->data);
		if(self->exit_req.after_work_cb) {
			self->exit_req.after_work_cb(&self->exit_req, 0);
		}
	}
}
