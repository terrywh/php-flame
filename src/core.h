#pragma once

class task_runner;
class keeper;
class core {
public:
	// 需要在 module 的 startup 流程中启动 shutdown 流程中销毁
	// 内部任务队列引用了 PHP 对象，这些对象会在 shutdown 之前被 PHP 主动销毁
	static event_base*  base;
	static task_runner* task;
	static evdns_base*  base_dns;
	static keeper*      keep;
	static std::size_t  count;
	static void init(php::extension_entry& extension);
	static void stop();
	static php::value make_exception(const boost::format& message, int code = 0) {
		return php::make_exception(message.str(), code);
	}
	struct generator_wrapper {
		php::generator  gn;
		event           ev;
		php::callable   cb;
	};
	static generator_wrapper* generator_start(const php::generator& gn);
private:
	static php::value go(php::parameters& params);
	static php::value run(php::parameters& params);
	static php::value sleep(php::parameters& params);
	static php::value fork(php::parameters& params);
};
