#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class client: public php::class_base {
	public:
		client();
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value collection(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		mongoc_client_t* client_;
		mongoc_uri_t*    uri_;
		void connect_();
		static void default_cb(uv_work_t* req, int status);
		static void connect_wk(uv_work_t* req);
		static void collection_wk(uv_work_t* req);
		static void close_wk(uv_work_t* req);
	};
}
}
}
