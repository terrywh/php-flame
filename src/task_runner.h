#pragma once
class task_wrapper {
public:
	task_wrapper(const std::function<void(php::callable)>& t,const php::callable& d);
	std::function<void(php::callable)> task;
	php::callable done;
	boost::asio::io_service::work work;
};
class task_runner {
public:
	task_runner();
	void start();
	void stop_wait();
	php::value async(const std::function<void(php::callable)>& task);
	inline bool is_master() {
		return std::this_thread::get_id() != master_id_;
	}
private:
	static void run(task_runner* t);
	std::thread                           thread_;
	// 目前没有限制任务生产者一定来自主线程，故不能使用 spsc_queue 类型队列
	boost::lockfree::queue<task_wrapper*> queue_;
	bool stopped_;
	const std::thread::id& master_id_;
};
