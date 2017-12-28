#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class client_implement;
	class client: public php::class_base {
	public:
		client();
		~client();
		php::value connect(php::parameters& params);
		php::value collection(php::parameters& params);
		
		client_implement* impl;
	private:
		static void connect_cb(uv_work_t* req, int status);
		static void collection_cb(uv_work_t* req, int status);
		static void default_cb(uv_work_t* req, int status);
	};
}
}
}
