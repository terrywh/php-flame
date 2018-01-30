#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../net/unix_socket.h"
#include "os.h"
#include "process.h"
#include "cluster/cluster.h"

namespace flame {
namespace os {
	static php::value executable(php::parameters& params) {
		php::string str(256);
		std::size_t len = 256;
		uv_exepath(str.data(), &len);
		str.resize(len);
		return std::move(str);
	}
	static php::value spawn(php::parameters& params) {
		php::object obj = php::object::create<process>();
		process*    cpp = obj.native<process>();
		cpp->__construct(params);
		return std::move(obj);
	}
	static void exec_cb(php::value& rv, coroutine* co, void* data) {
		co->next(rv);
	}
	php::value exec(php::parameters& params) {
		php::object proc1 = php::object::create<process>();
		process*    proc2 = proc1.native<process>();
    
		std::vector<php::value> args(3);
		args[0] = params[0];
		args[1] = params[1];
		
		php::array options;
		if(params.length() > 2 && params[2].is_array()) {
			options = params[2];
		}else{
			options = php::array(0);
		}
		options.at("stdout",6) = php::string("pipe",4);
		options.at("detach",6) = (zend_bool)false;
		args[2] = options;
		php::parameters argv(args);

		proc2->__construct(argv);
		net::unix_socket* out2 = proc2->stdout_.native<net::unix_socket>();
		// net::unix_socket* err2 = proc2->stderr_.native<net::unix_socket>();
		out2->rdr.read_all();
		// read_all 异步，在下面 exec_cb 中接收其返回值
		coroutine::current->async(exec_cb, nullptr);
		// !!! 若需要 stderr 应在 exec_cb 中启动 err2->rdr.read_all()
		// 并再加入一个 yield 回调函数
		return flame::async(proc2);
	}
	void init(php::extension_entry& ext) {
		ext.add<executable>("flame\\os\\executable");
		ext.add<spawn>("flame\\os\\spawn");
		ext.add<exec>("flame\\os\\exec");
		php::class_entry<process> class_process("flame\\os\\process");
		class_process.add(php::property_entry("pid", int(0)));
		class_process.add<&process::__construct>("__construct");
		class_process.add<&process::kill>("kill");
		class_process.add<&process::wait>("wait");
		class_process.add<&process::send>("send");
		class_process.add<&process::ondata>("ondata");
		class_process.add<&process::stdout>("stdout");
		class_process.add<&process::stderr>("stderr");
		ext.add(std::move(class_process));

		cluster::init(ext);
	}
}
}
