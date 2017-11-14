#pragma once

namespace flame {
	class thread_worker {
	public:
		thread_worker();
		void queue_work(uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb);
		~thread_worker();
	private:
		void join();
		void run();
		void master_cb();
		void worker_cb();
		static void run(void* data);
		static void master_cb(uv_async_t* handle);
		static void worker_cb(uv_async_t* handle);

		uv_loop_t   loop_;
		bool        exit_;
		uv_thread_t tid_;
		uv_async_t  async_worker;
		uv_async_t  async_master;
		uv_mutex_t  mutex_worker;
		uv_mutex_t  mutex_master;
		std::deque<uv_work_t*> queue_worker;
		std::deque<uv_work_t*> queue_master;
	};
}
