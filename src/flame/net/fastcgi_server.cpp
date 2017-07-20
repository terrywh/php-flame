// #include "vendor.h"
// #include "fastcgi_server.h"
// #include "fastcgi_session.h"

// void fastcgi_server::init(php::extension_entry& extension) {
// 	php::class_entry<fastcgi_server> class_("flame\\fastcgi_server");
// 	class_.add<&fastcgi_server::listen>("listen");
// 	class_.add<&fastcgi_server::run>("run");
// 	extension.add(std::move(class_));
// }

// php::value fastcgi_server::listen(php::parameters& params) {
// 	php::string& path = params[0];
// 	// 重新监听
// 	unlink(path.c_str());
// 	sock_ = mill_unixlisten(path.c_str(), 1024);
// 	if(errno != 0) {
// 		throw php::exception(strerror(errno), errno);
// 	}
// 	// 允许其他用户访问、连接
// 	chmod(path.c_str(), 0777);
// 	return nullptr;
// }

// mill_fiber static void session_handler(fastcgi_session* sess) {
// 	sess->run();
// }

// php::value fastcgi_server::run(php::parameters& params) {
// 	while(true) {
// 		mill_unixsock sock = mill_unixaccept(sock_, -1);
// 		// TODO 使用内存池创建 fastcgi_session 对象？
// 		fastcgi_session* sess = new fastcgi_session(sock);
// 		// 每个会话启用单独的协程进行处理
// 		mill_go(session_handler(sess));
// 	}
// 	return nullptr;
// }
