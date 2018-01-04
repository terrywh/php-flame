#pragma once

namespace flame {
namespace db {
namespace kafka {
	struct consumer_request_t;
	class consumer_implement;
	class consumer: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value consume(php::parameters& params);
		php::value commit(php::parameters& params);
		php::value __destruct(php::parameters& params);
		consumer_implement* impl;
	private:
		consumer_request_t* ctx_;
		static void consume_cb(uv_work_t* req, int status);
		static void default_cb(uv_work_t* req, int status);
		
		friend class message;
	};
}
}
}
