#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class client;
	class collection_implement;
	class collection: public php::class_base {
	public:
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
		php::value __destruct(php::parameters& params);
		void init(thread_worker* worker, client* cli, mongoc_collection_t* collection);
		php::value count(php::parameters& params);
		php::value insert_one(php::parameters& params);
		php::value insert_many(php::parameters& params);
		php::value remove_one(php::parameters& params);
		php::value remove_many(php::parameters& params);
		php::value update_one(php::parameters& params);
		php::value update_many(php::parameters& params);
		php::value find_one(php::parameters& params);
		php::value find_many(php::parameters& params);
		collection_implement* impl;
	private:
		php::object ref_;
		static void insert_many_cb(uv_work_t* w, int status);
		static void find_one_cb(uv_work_t* w, int status);
		static void find_many_cb(uv_work_t* w, int status);
		static void default_cb(uv_work_t* w, int status);
		static void boolean_cb(uv_work_t* w, int status);
	};
}
}
}
