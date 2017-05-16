#pragma once

struct task_wrapper {
	event                              ev;
	php::callable                      fn;
	php::callable                      cb;
	php::value                         ex;
	php::value                         rv;
};
class task_runner {
public:
	static void init(php::extension_entry& extension);
	task_runner();
	~task_runner();
	static php::value async(php::parameters& params);
	inline bool is_master() {
		return std::this_thread::get_id() != master_id_;
	}
	void start();
	void wait();
	void stop();
private:
	static void async_todo(evutil_socket_t fd, short events, void* data);
	static void async_done(evutil_socket_t fd, short events, void* data);
	static void run(task_runner* self);
	event_base*            base_;
	event*                 ev_;
	std::thread            worker_;
	const std::thread::id& master_id_;
};
