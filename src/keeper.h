#pragma once

struct keeper_wrapper {
	php::object    conn;
	std::string    ping;
	struct timeval time;
	int            ttl;
};
// keeper 负责管理 mysql 、redis、mongodb 连接维护连接的活跃（防止服务端断开）
class keeper: public php::class_base {
public:
	static void init(php::extension_entry& extension);
	static php::value keep(php::parameters& params);
	static php::value take(php::parameters& params);
	keeper();
	~keeper();
	void start();
	void stop();
private:
	event   ev_;
	std::map<zend_object*, keeper_wrapper> map_;
	static void timer_handler(evutil_socket_t fd, short events, void* data);
};
