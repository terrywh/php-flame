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
	void stop_wait();
	php::value async(const std::function<void(php::callable)>& task);
	inline int concurrency() {
		return thread_.size();
	}
private:
	static void run(task_runner* t);
	std::array<std::thread, 1>            thread_;
	boost::lockfree::queue<task_wrapper*> queue_;
	bool stopped_;
};
