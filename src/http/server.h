#pragma once

namespace flame {
namespace http {
	class acceptor;
	class server: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value before(php::parameters& params);
		php::value after(php::parameters& params);
		php::value put(php::parameters& params);
		php::value delete_(php::parameters& params);
		php::value post(php::parameters& params);
		php::value get(php::parameters& params);
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		tcp::endpoint  addr_;
		std::map<std::string, php::callable> cb_;
		std::shared_ptr<acceptor> acc_;
		friend class acceptor;
		friend class handler;
	};
}
}