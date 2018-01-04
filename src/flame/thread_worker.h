#pragma once

namespace flame {
	class thread_worker {
	public:
		typedef void (*join_cb_t)(void* data);
		thread_worker();
		void queue_work(uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb);
		void close_work(void* data, uv_work_cb work_cb, uv_after_work_cb after_work_cb);
		
		uv_loop_t   loop;
		
		uv_mutex_t* worker_lock();
	private:
		static void run(void* data);
		static void master_cb(uv_async_t* handle);
		static void worker_cb(uv_async_t* handle);
		static void worker_close_cb(uv_handle_t* handle);
		static void master_close_cb(uv_handle_t* handle);
		
		uv_work_t   exit_req;
		uv_thread_t tid_;
		uv_async_t  async_worker;
		uv_async_t  async_master;
		uv_mutex_t  mutex_worker;
		uv_mutex_t  mutex_master;
		std::deque<uv_work_t*> queue_worker;
		std::deque<uv_work_t*> queue_master;
	};
}
