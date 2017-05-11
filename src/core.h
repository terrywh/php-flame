#pragma once
class task_runner;
class core {
public:
	// io_servcie 需要在 module 的 startup 流程中启动 shutdown 流程中销毁
	// 内部任务队列引用了 PHP 对象，这些对象会在 shutdown 之前被 PHP 主动销毁
	static inline boost::asio::io_service& io() {
		return *io_;
	}
	static inline task_runner& tr() {
		return *tr_;
	}
	static void init(php::extension_entry& extension);
	static php::value error_to_exception(const boost::system::error_code& err);
	static php::value error(const std::string& message, int code = 0);
private:
	static boost::asio::io_service* io_;
	static task_runner*             tr_;
	static php::value go(php::parameters& params);
	static php::value run(php::parameters& params);
	static php::value sleep(php::parameters& params);
	static php::value async(php::parameters& params);
	static php::value _fork(php::parameters& params);
};
